#!/usr/bin/python3
print("/*\n"
      " * Generated using scanline_generator.py\n"
      " */")

for byte in range(0, 20):
    print("/* Pixels " + str(byte * 8) + "-" + str(((byte + 1) * 8) - 1) + " */")
    print("pxs = *(uint32_t *) (line + (" + str(byte) + " * BUFFER_BPP));")
    for pixel in range(8):
        print("GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (" + str(7 - pixel) + " * BUFFER_BPP)) & PIXEL_MASK];")
        print("__asm__ volatile(\"nop\");")
        print("__asm__ volatile(\"nop\");")
        if pixel % 2 == 0:
            print("__asm__ volatile(\"nop\");")
        if pixel != 0 and pixel % 3 == 0:
            print("__asm__ volatile(\"nop\");")
        if pixel == 1:
            print("__asm__ volatile(\"nop\");")
            print("__asm__ volatile(\"nop\");")
        if pixel == 4:
            print("__asm__ volatile(\"nop\");")

print("__asm__ volatile(\"nop\");")
print("__asm__ volatile(\"nop\");")
print("__asm__ volatile(\"nop\");")
print("__asm__ volatile(\"nop\");")
print("__asm__ volatile(\"nop\");")
print("__asm__ volatile(\"nop\");")
print("__asm__ volatile(\"nop\");")
print("__asm__ volatile(\"nop\");")
print("__asm__ volatile(\"nop\");")
print("__asm__ volatile(\"nop\");")
