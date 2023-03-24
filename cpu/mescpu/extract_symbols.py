#!/usr/bin/env python3

import sys
from elftools.elf.elffile import ELFFile

INVALID_SYMBOLS = ["siprintf", "hello_world", "main", "vector_table", "get_lot_base", "null_handler",
                   "blocking_handler", "errno", "impure_data", "error_codes", "module_table", "offset_sym",
                   "get_data_pointer", "debug_level", "udynlink_debug", "get_code_pointer"]

EXTRA_SYMBOLS = ["__aeabi_dadd",
                 "__aeabi_ddiv",
                 "__aeabi_dmul",
                 "__aeabi_drsub",
                 "__aeabi_dsub",
                 "__aeabi_cdcmpeq",
                 "__aeabi_cdcmple",
                 "__aeabi_cdrcmple",
                 "__aeabi_dcmpeq",
                 "__aeabi_dcmplt",
                 "__aeabi_dcmple",
                 "__aeabi_dcmpge",
                 "__aeabi_dcmpgt",
                 "__aeabi_dcmpun",
                 "__aeabi_fadd",
                 "__aeabi_fdiv",
                 "__aeabi_fmul",
                 "__aeabi_frsub",
                 "__aeabi_fsub",
                 "__aeabi_cfcmpeq",
                 "__aeabi_cfcmple",
                 "__aeabi_cfrcmple",
                 "__aeabi_fcmpeq",
                 "__aeabi_fcmplt",
                 "__aeabi_fcmple",
                 "__aeabi_fcmpge",
                 "__aeabi_fcmpgt",
                 "__aeabi_fcmpun",
                 "__aeabi_d2iz",
                 "__aeabi_d2uiz",
                 "__aeabi_d2lz",
                 "__aeabi_d2ulz",
                 "__aeabi_f2iz",
                 "__aeabi_f2uiz",
                 "__aeabi_f2lz",
                 "__aeabi_f2ulz",
                 "__aeabi_d2f",
                 "__aeabi_f2d",
                 # "__aeabi_h2f",
                 # "__aeabi_h2f_alt",
                 # "__aeabi_f2h",
                 # "__aeabi_f2h_alt",
                 # "__aeabi_d2h",
                 # "__aeabi_d2h_alt",
                 "__aeabi_i2d",
                 "__aeabi_ui2d",
                 "__aeabi_l2d",
                 "__aeabi_ul2d",
                 "__aeabi_i2f",
                 "__aeabi_ui2f",
                 "__aeabi_l2f",
                 "__aeabi_ul2f",
                 "__aeabi_lmul",
                 "__aeabi_ldivmod",
                 "__aeabi_uldivmod",
                 "__aeabi_llsl",
                 "__aeabi_llsr",
                 "__aeabi_lcmp",
                 "__aeabi_ulcmp",
                 "__aeabi_idiv",
                 "__aeabi_uidiv",
                 "__aeabi_idivmod",
                 "__aeabi_uidivmod",
                 "__aeabi_idiv0",
                 "__aeabi_ldiv0",
                 "__aeabi_uread4",
                 "__aeabi_uwrite4",
                 "__aeabi_uread8",
                 "__aeabi_uwrite8",
                 "__aeabi_memcpy8",
                 "__aeabi_memcpy4",
                 "__aeabi_memcpy",
                 "__aeabi_memmove8",
                 "__aeabi_memmove4",
                 "__aeabi_memmove",
                 "__aeabi_memset8",
                 "__aeabi_memset4",
                 "__aeabi_memset",
                 "__aeabi_memclr8",
                 "__aeabi_memclr4",
                 "__aeabi_memclr"]

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
                symbols.append(symbol.name)

e_symbols = [*symbols, *EXTRA_SYMBOLS]

for symbol in e_symbols:
    print(
        f"if (strcmp(name, \"{symbol}\") == 0)\n\treturn (uint32_t)&{symbol};")
