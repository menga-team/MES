#!/usr/bin/python3
BUFFER_A_BASE = 0x20000000
BUFFER_B_BASE = 0x20001c20
BUFFER_WIDTH = 120
BPP = 3


def get_block_addresses(base_adr, bpp):
    blocks = []
    for line in range(120):
        for block in range(20):
            blocks.append("(uint32_t*)" + str(hex(int(
                (base_adr + (line * (BUFFER_WIDTH / 8) * bpp)) + block * bpp
            ))))
    return blocks


print("/*\n"
      " * Generated using pixel_block_generator.py\n"
      " */")

print("volatile const __attribute__((section(\".rodata\"))) uint32_t *buffer_a_blocks[2400] = {")
print(", ".join(get_block_addresses(BUFFER_A_BASE, BPP)))
print("};")
print("volatile const __attribute__((section(\".rodata\"))) uint32_t *buffer_b_blocks[2400] = {")
print(", ".join(get_block_addresses(BUFFER_B_BASE, BPP)))
print("};")
