#!/usr/bin/python3
print("/*\n"
      " * Generated using scanline_generator.py\n"
      " */")

for byte in range(0, 20):
    print("/* Pixels " + str(byte * 8) + "-" + str((byte + 1) * 8) + " */ pxs = *(uint32_t *) (line + (" + str(byte) + " * BUFFER_BPP));")
    for pixel in range(8):
        print("GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> ((7 -" + str(pixel) + ") * BUFFER_BPP)) & PIXEL_MASK];")
        if pixel != 7:
            print("__asm__(\"nop\");")
        if pixel % 4 == 0:
            print("__asm__(\"nop\");")
        if pixel == 1:
            print("__asm__(\"nop\");")
