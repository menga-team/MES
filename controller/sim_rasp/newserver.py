#!/usr/bin/env python3

import pygame
from pygame_button import Button
import time
import serial

s = serial.Serial("/dev/ttyACM0", 9600)
last_first_bit = None

while True:
    data = s.read(2)
    if data[0] != last_first_bit:
    	print(data[1], end="")
    last_first_bit = data[0]
