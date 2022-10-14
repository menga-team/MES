#!/usr/bin/python3

print("pxs = *(uint32_t *) (line + (0 * BUFFER_BPP));")

for byte in range(1, 21):
    for pixel in range(8):
        print("GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (" + str(pixel) + " * BUFFER_BPP)) & PIXEL_MASK];",
              end="")
        if pixel == 5:
            print("adr = (" + str(byte) + " * BUFFER_BPP)")
        if pixel == 6:
            print("adr += line;", end="")
        if pixel == 7:
            print("pxs = *(uint32_t *) adr;", end="")
        else:
            pass
    print("")
