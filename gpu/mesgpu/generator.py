#!/usr/bin/python3
print("/*\n"
      " * Generated using generator.py\n"
      " */")
for byte in range(0, 20):
    for pixel in range(8):
        print("GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(scan_line["
              + str(byte) + "] >> (" + str(pixel) + " * BUFFER_BPP)) & PIXEL_MASK];", end="")
        if (byte == 4 and pixel == 7) or (byte == 5 and pixel == 0) \
                or (byte == 8 and pixel == 7) or (byte == 9 and pixel == 0) \
                or (byte == 9 and pixel == 7) or (byte == 10 and pixel == 0) \
                or (byte == 10 and pixel == 7) or (byte == 11 and pixel == 0) \
                or (byte == 11 and pixel == 7) or (byte == 12 and pixel == 0) \
                or (byte == 12 and pixel == 7) or (byte == 13 and pixel == 0) \
                or (byte == 13 and pixel == 7) or (byte == 14 and pixel == 0) \
                or (byte == 14 and pixel == 7) or (byte == 15 and pixel == 0) \
                or (byte == 15 and pixel == 7) or (byte == 16 and pixel == 0) \
                or (byte == 16 and pixel == 7) or (byte == 17 and pixel == 0) \
                or (byte == 17 and pixel == 7) or (byte == 18 and pixel == 0) \
                or (byte == 18 and pixel == 7) or (byte == 19 and pixel == 0) \
                or (byte == 19 and pixel == 7):
            print("__asm(\"nop\");", end="")
        else:
            for x in range(2):
                print("__asm(\"nop\");", end="")
    print("")
