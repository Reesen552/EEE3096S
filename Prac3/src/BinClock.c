/*
 * BinClock.c
 * Jarrod Olivier
 * Modified for EEE3095S/3096S by Keegan Crankshaw
 * August 2019
 * 
 * GVNREE003 ENRMOH001
 * Date 18/08/2019
*/

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h> //For printf functions
#include <stdlib.h> // For system functions
#include <softPwm.h> //For pwn functionality
#include <signal.h> //For handling keyboard interrupts
#include <math.h>

#include "BinClock.h"
#include "CurrentTime.h"

//Global variables
int hours, mins, secs;
long lastInterruptTime = 0; //Used for button debounce
int RTC; //Holds the RTC instance
static volatile int run = 1; //Used for catching keyboard interrupt

int HH,MM,SS;
int hourLEDs[] = {0, 1, 4, 6};
int minLEDs[] = {2, 3, 21, 22, 23, 25};

void cleanup(){
	softPwmWrite(26,0);
	for(int i=0;i<4;i++){
		digitalWrite(hourLEDs[i], 0);
	}
	for(int i=0;i<6;i++){
		digitalWrite(minLEDs[i], 0);
	}
	delay(100);
}

void handler(int placeholder){
	run = 0;
	cleanup();
	exit(0);
}

void initGPIO(void){
	/* 
	 * Sets GPIO using wiringPi pins. see pinout.xyz for specific wiringPi pins
	 * You can also use "gpio readall" in the command line to get the pins
	 * Note: wiringPi does not use GPIO or board pin numbers (unless specifically set to that mode)
	 */
	printf("Setting up\n");
	wiringPiSetup(); //This is the default mode. If you want to change pinouts, be aware
	
	RTC = wiringPiI2CSetup(RTCAddr); //Set up the RTC
	//Set up the LEDS
	for(int i=0; i < 4; i++){
		pinMode(hourLEDs[i], OUTPUT);
	}
	for(int i=0; i < 6; i++){
		pinMode(minLEDs[i], OUTPUT);
	}
	
	//Set Up the Seconds LED for PWM
	softPwmCreate(26,0,60);
	
	printf("LEDS done\n");
	
	//Set up the Buttons
	for(int j=0; j < sizeof(BTNS)/sizeof(BTNS[0]); j++){
		pinMode(BTNS[j], INPUT);
		pullUpDnControl(BTNS[j], PUD_UP);
	}
	
	//Attach interrupts to Buttons
	wiringPiISR(27, INT_EDGE_RISING, &hourInc);
	wiringPiISR(7, INT_EDGE_RISING, &minInc);
	
	printf("BTNS done\n");
	printf("Setup done\n");
}


/*
 * The main function
 * This function is called, and calls all relevant functions we've written
 */
int main(void){
	initGPIO();
;
	toggleTime();

	signal(SIGINT, handler);
	// Repeat this until we shut down
	while(run){
		//Fetch the time from the RTC
		hours = wiringPiI2CReadReg8(RTC, HOUR);
		mins = wiringPiI2CReadReg8(RTC, MIN);
		secs = wiringPiI2CReadReg8(RTC, SEC);
		
		//Function calls to toggle LEDs
		int rtc_hours = formatHours(hours);
		lightHours(rtc_hours);
		
		int rtc_mins = formatMinutes(mins);
		lightMins(rtc_mins);
		
		int rtc_secs = formatSeconds(secs);
		secPWM(rtc_secs);

		printf("The current time is: %d:%d:%d\n", formatHours(hours), formatMinutes(mins), formatSeconds(secs));
		
		//using a delay to make our program "less CPU hungry"
		delay(1000); //milliseconds
	}
	return 0;
}

/*
 * Change the hour format to 12 hours
 */
int hFormat(int hours){
	/*formats to 12h*/
	if (hours >= 24){
		hours = 0;
	}
	else if (hours > 12){
		hours -= 12;
	}
	return (int)hours;
}

int formatHours(int input){
	int hourTen = input & 0b00010000;
	int hourOne = input & 0b00001111;
	if(hourTen){
		return 10+hourOne;
	}
	else{
		return hourOne;
	}
}

int formatMinutes(int input){
	int minuteTen = (input & 0b01110000)>>4;
	int minuteOne = input & 0b00001111;
	return minuteTen*10 + minuteOne;
}

int formatSeconds(int input){
	int secondTen = (input & 0b01110000)>>4;
	int secondOne = input & 0b00001111;
	return secondTen*10 + secondOne;
}

/*
 * Turns on corresponding LED's for hours
 */
void lightHours(int units){
	// Write your logic to light up the hour LEDs here
	for(int i=0; i < 4; i++){
		digitalWrite(hourLEDs[i], units & 1);
		units /= 2;
	}
}

/*
 * Turn on the Minute LEDs
 */
void lightMins(int units){
	//Write your logic to light up the minute LEDs here
	int mns = hexCompensation(units);
	for(int i=0; i<6; i++){
		digitalWrite(minLEDs[i], mns & 1);
		mns /= 2;
	}
}

/*
 * PWM on the Seconds LED
 * The LED should have 60 brightness levels
 * The LED should be "off" at 0 seconds, and fully bright at 59 seconds
 */
void secPWM(int seconds){
	// Write your logic here
	softPwmWrite(26,seconds);
}

/*
 * hexCompensation
 * This function may not be necessary if you use bit-shifting rather than decimal checking for writing out time values
 */
int hexCompensation(int units){
	/*Convert HEX or BCD value to DEC where 0x45 == 0d45 
	  This was created as the lighXXX functions which determine what GPIO pin to set HIGH/LOW
	  perform operations which work in base10 and not base16 (incorrect logic) 
	*/
	int unitsU = units%0x10;

	if (units >= 0x50){
		units = 50 + unitsU;
	}
	else if (units >= 0x40){
		units = 40 + unitsU;
	}
	else if (units >= 0x30){
		units = 30 + unitsU;
	}
	else if (units >= 0x20){
		units = 20 + unitsU;
	}
	else if (units >= 0x10){
		units = 10 + unitsU;
	}
	return units;
}


/*
 * decCompensation
 * This function "undoes" hexCompensation in order to write the correct base 16 value through I2C
 */
int decCompensation(int units){
	int unitsU = units%10;

	if (units >= 50){
		units = 0x50 + unitsU;
	}
	else if (units >= 40){
		units = 0x40 + unitsU;
	}
	else if (units >= 30){
		units = 0x30 + unitsU;
	}
	else if (units >= 20){
		units = 0x20 + unitsU;
	}
	else if (units >= 10){
		units = 0x10 + unitsU;
	}
	return units;
}


/*
 * hourInc
 * Fetch the hour value off the RTC, increase it by 1, and write back
 * Be sure to cater for there only being 23 hours in a day
 * Software Debouncing should be used
 */
void hourInc(void){
	//Debounce
	long interruptTime = millis();

	int hr;
	if (interruptTime - lastInterruptTime>200){
		
		//Fetch RTC Time
		hr = hexCompensation(wiringPiI2CReadReg8(RTC, HOUR));
		
		printf("Button pressed 1: Hours increased by 1\n");
		
		//Increase hours by 1, ensuring not to overflow
		if(hFormat(hr)>11){
			hr = 1;
		}
		else{
			hr+=1;
		}
		//Write hours back to the RTC
		wiringPiI2CWriteReg8(RTC, HOUR, decCompensation(hr));
	}
	lastInterruptTime = interruptTime;
}

/* 
 * minInc
 * Fetch the minute value off the RTC, increase it by 1, and write back
 * Be sure to cater for there only being 60 minutes in an hour
 * Software Debouncing should be used
 */
void minInc(void){
	long interruptTime = millis();

	int mn;
	int hr;
	if (interruptTime - lastInterruptTime>200){
		//Fetch RTC Time
		mn = hexCompensation(wiringPiI2CReadReg8(RTC, MIN));
		hr = hexCompensation(wiringPiI2CReadReg8(RTC, HOUR));
		
		printf("Button pressed 1: Minutes increased by 1\n");
		
		//Increase minutes by 1, ensuring overflow to hours
		if(mn==59){
			mn = 0;
			if(hFormat(hr)>11){
				hr = 1;
			}
			else{
				hr+=1;
			}
		}
		else{
			mn +=1;
		}
		//Write minutes back to the RTC
		wiringPiI2CWriteReg8(RTC, MIN, decCompensation(mn));
		wiringPiI2CWriteReg8(RTC, HOUR, decCompensation(hr));
	}
	lastInterruptTime = interruptTime;
}

//This interrupt will fetch current time from another script and write it to the clock registers
//This functions will toggle a flag that is checked in main
void toggleTime(void){
	
		HH = getHours();
		MM = getMins();
		SS = getSecs();

		HH = hFormat(HH);
		HH = decCompensation(HH);
		wiringPiI2CWriteReg8(RTC, HOUR, HH);

		MM = decCompensation(MM);
		wiringPiI2CWriteReg8(RTC, MIN, MM);

		SS = decCompensation(SS);
		wiringPiI2CWriteReg8(RTC, SEC, 0b10000000+SS);

}
