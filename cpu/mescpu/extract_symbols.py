#!/usr/bin/env python3

import sys
from elftools.elf.elffile import ELFFile

INVALID_SYMBOLS = ["siprintf", "hello_world", "main", "vector_table", "get_lot_base", "null_handler",
                   "blocking_handler", "errno", "impure_data", "error_codes", "module_table", "offset_sym",
                   "get_data_pointer", "debug_level", "udynlink_debug", "get_code_pointer"]

elf_file = sys.argv[1]

symbols = []

with open(elf_file, "rb") as f:
    elf = ELFFile(f)
    for section in elf.iter_sections(type="SHT_SYMTAB"):
        for symbol in section.iter_symbols():
            s_type = symbol["st_info"]["type"]
            if s_type == "STT_FUNC" or s_type == "STT_OBJECT":
                if symbol.name in INVALID_SYMBOLS:
                    continue
                if '.' in symbol.name:
                    continue
                if str(symbol.name).startswith("_"):
                    continue
                symbols.append(symbol)

for symbol in symbols:
    print(f"if (strcmp(name, \"{symbol.name}\") == 0)\n\treturn (uint32_t)&{symbol.name};")

print("return 0;")
