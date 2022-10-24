import sys
import utime

led = machine.Pin(25, machine.Pin.OUT)
clock_out = machine.Pin(17, machine.Pin.OUT)
data_in = machine.Pin(26, machine.Pin.IN)

# led.value(1)
# utime.sleep(2)
cycle = 0
wait = 0.005
last_three_bits = []

# while True:
# 	cycle += 1
# 	bit = data_in.value()
# 	# sys.stdout.write("1" + str(bit) + " ")
# 	sys.stdout.write(str(bit))
# 	utime.sleep(wait)

# 	clock_out.value(0)
# 	led.value(0)
# 	utime.sleep(wait)

# 	bit = data_in.value()
# 	# sys.stdout.write(str(bit))
# 	utime.sleep(wait)

# 	clock_out.value(1)
# 	led.value(1)
# 	utime.sleep(wait)

# 	last_three_bits += [bit]
# 	if len(last_three_bits) > 3:
# 		last_three_bits = last_three_bits[1:]
# 	if all(last_three_bits):
# 	# if cycle % 32 == 0:
# 		sys.stdout.write("\n")

while True:
	# cycle += 1
	# bit = data_in.value()
	# sys.stdout.write("1" + str(bit) + " ")
	# sys.stdout.write(str(bit))
	# utime.sleep(wait)

	clock_out.value(0)
	led.value(0)
	# utime.sleep(wait)

	# bit = data_in.value()
	# sys.stdout.write(str(bit))
	# utime.sleep(wait)

	clock_out.value(1)
	led.value(1)
	# utime.sleep(wait)

	# last_three_bits += [bit]
	# if len(last_three_bits) > 3:
		# last_three_bits = last_three_bits[1:]
	# if all(last_three_bits):
	# if cycle % 32 == 0:
		# sys.stdout.write("\n")
