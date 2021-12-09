import RPi.GPIO as GPIO
# import ConfigParser

pin_number = 17

GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)
GPIO.setup(pin_number, GPIO.OUT)
GPIO.output(pin_number, GPIO.HIGH)
