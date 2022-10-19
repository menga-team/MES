import sys
import utime
import _thread

led = machine.Pin(25, machine.Pin.OUT)
clock_out = machine.Pin(17, machine.Pin.OUT)
data_out = machine.Pin(26, machine.Pin.OUT)
clock_in = machine.Pin(14, machine.Pin.IN)
data_in = machine.Pin(15, machine.Pin.IN)

button = 0
cycles = 0
data = "000000001"

def serial_stuff():
    while True:
        v = sys.stdin.readline().strip()
        if len(v) < 9:
            print(len(v))
            continue
        print(v)
        sys.stdin.readline().strip()
        data = v

print("ready")
sys.stdin.readline().strip()

_thread.start_new_thread(serial_stuff, ())

while True:
    if data == "1":
        clock_out.value(1)
        data_output = 0
            
        if cycles == 0:
            data_output = 1
            cycles += 1
        elif cycles == 1:
            if button == 8:
                data_output = 1
            elif data[button] == "1":
                data_output = 1
            elif data[button] == "0":
                data_output = 0
            cycles += 1
        elif cycles == 2:
            if button == 8:
                data_output = 1
                button = 0
            else:
                button += 1
                data_output = 0
            cycles = 0
        
        # print(button, cycles, data_output) 
        data_out.value(data_output)  
    elif data[8] == "0":
        clock_out.value(0)

    utime.sleep(0.3)