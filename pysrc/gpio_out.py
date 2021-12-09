import RPi.GPIO as GPIO
import time

relais_pin = 17
button_pin = 22

def logit(n): 
	if GPIO.input(n):
		print("opened")
		GPIO.output(relais_pin, GPIO.HIGH)
	else:
		print("closed")
		GPIO.output(relais_pin, GPIO.LOW)


GPIO.setmode(GPIO.BCM)
GPIO.setup(button_pin, GPIO.IN, pull_up_down = GPIO.PUD_DOWN)
GPIO.setup(relais_pin, GPIO.OUT)
GPIO.add_event_detect(button_pin, GPIO.BOTH, callback=logit)

while True:
# 	print(GPIO.input(button_pin))
 	time.sleep(1)

GPIO.cleanup()
