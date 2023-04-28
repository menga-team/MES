#!/usr/bin/env python3

import os
import sys

bin_file = sys.argv[1]

with open(bin_file, "rb") as asset:
    print("#include <stdint.h>\n");
    print('const uint8_t __attribute__((section(".game_arr"))) hello_world[] __attribute__((aligned(4))) = {');
    print("\t", end="")
    while (byte := asset.read(1)):
        print(f"0x{byte.hex()}, ", end="")
    print("\n};\n")

