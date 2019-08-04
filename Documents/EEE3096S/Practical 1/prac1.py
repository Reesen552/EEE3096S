

# import Relevant Librares
import RPi.GPIO as GPIO
import time
import itertools

GPIO.setmode(GPIO.BCM)

LED_config = [18,23,24]
SW1 = 17
SW2 = 27
count = 0
i=list(itertools.product([0,1], repeat=3))

GPIO.setup(18, GPIO.OUT)
GPIO.setup(23, GPIO.OUT)
GPIO.setup(24, GPIO.OUT)
GPIO.setup(SW1, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(SW2, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.output(18, GPIO.HIGH)
GPIO.output(23, GPIO.HIGH)
GPIO.output(24, GPIO.HIGH)

# Logic that you write
def main():
    print(i)
    while True:
        time.sleep(1)
        GPIO.add_event_detect(SW2, GPIO.FALLING, callback=my_callback_one, bouncetime = 250)
        GPIO.add_event_detect(SW1, GPIO.FALLING, callback=my_callback_two, bouncetime = 250)


def my_callback_one(channel):
    global count
    if count == 7:
        count=0
    else:
        count += 1
    GPIO.output(LED_config, i[count])

def my_callback_two(channel):
    global count
    if count == 0:
        count=7
    else:
        count -= 1
    GPIO.output(LED_config, i[count])



# Only run the functions if
if __name__ == "__main__":
    # Make sure the GPIO is stopped correctly
    try:
        while True:
            main()
    except KeyboardInterrupt:
        print("Exiting gracefully")
        GPIO.output(18, GPIO.LOW)
        GPIO.output(23, GPIO.LOW)
        GPIO.output(24, GPIO.LOW)
        # Turn off your GPIOs here
        GPIO.cleanup()
   # except Exception as e:
    #    GPIO.cleanup()
     #   print("Some other error occurred")
      #  print(e.message)