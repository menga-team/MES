#!/usr/bin/env python3

import sys
import os

asset_file = sys.argv[1]
asset_size = 0

with open(asset_file, "rb") as asset:
    base_name =  os.path.basename(asset_file)
    variable = base_name.upper().replace(".", "_")
    print("#include <stdint.h>\n");
    print(f"static const uint8_t ASSET_{variable}[] = {{");
    print("\t", end="")
    while (byte := asset.read(1)):
        asset_size += 1
        print(f"0x{byte.hex()}, ", end="")
    print("\n};\n")
    print(f"static const uint32_t ASSET_{variable}_SIZE = {asset_size};")
    
