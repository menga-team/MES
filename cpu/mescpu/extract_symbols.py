#!/usr/bin/env python3

import sys
from elftools.elf.elffile import ELFFile

INVALID_SYMBOLS = ["siprintf", "hello_world", "main", "vector_table",
                   "get_lot_base", "null_handler", "blocking_handler",
                   "errno", "impure_data", "error_codes",
                   "module_table", "offset_sym", "get_data_pointer",
                   "debug_level", "udynlink_debug",
                   "get_code_pointer", "enter_hard_fault_handler",
                   "CONTROLLER_PIN_MAP", "BUTTON_CHARACTERS",
                   "period_certification", "atanhi", "atanlo", "halF",
                   "ln2HI", "ln2LO", "Zero", "npio2_hw", "Pio2",
                   "init_jk", "with_errno", "xflow", "finite"]

EXTRA_SYMBOLS = ["__aeabi_dadd", "__aeabi_ddiv", "__aeabi_dmul",
                 "__aeabi_drsub", "__aeabi_dsub", "__aeabi_cdcmpeq",
                 "__aeabi_cdcmple", "__aeabi_cdrcmple",
                 "__aeabi_dcmpeq", "__aeabi_dcmplt", "__aeabi_dcmple",
                 "__aeabi_dcmpge", "__aeabi_dcmpgt", "__aeabi_dcmpun",
                 "__aeabi_fadd", "__aeabi_fdiv", "__aeabi_fmul",
                 "__aeabi_frsub", "__aeabi_fsub", "__aeabi_cfcmpeq",
                 "__aeabi_cfcmple", "__aeabi_cfrcmple",
                 "__aeabi_fcmpeq", "__aeabi_fcmplt", "__aeabi_fcmple",
                 "__aeabi_fcmpge", "__aeabi_fcmpgt", "__aeabi_fcmpun",
                 "__aeabi_d2iz", "__aeabi_d2uiz", "__aeabi_d2lz",
                 "__aeabi_d2ulz", "__aeabi_f2iz", "__aeabi_f2uiz",
                 "__aeabi_f2lz", "__aeabi_f2ulz", "__aeabi_d2f",
                 "__aeabi_f2d", "__aeabi_ui2d", "__aeabi_l2d",
                 "__aeabi_ul2d", "__aeabi_i2f", "__aeabi_ui2f",
                 "__aeabi_l2f", "__aeabi_ul2f", "__aeabi_lmul",
                 "__aeabi_ldivmod", "__aeabi_uldivmod",
                 "__aeabi_llsl", "__aeabi_llsr", "__aeabi_lcmp",
                 "__aeabi_ulcmp", "__aeabi_idiv", "__aeabi_uidiv",
                 "__aeabi_idivmod", "__aeabi_uidivmod",
                 "__aeabi_idiv0", "__aeabi_ldiv0", "__aeabi_uread4",
                 "__aeabi_uwrite4", "__aeabi_uread8",
                 "__aeabi_uwrite8", "__aeabi_memcpy8",
                 "__aeabi_memcpy4", "__aeabi_memcpy",
                 "__aeabi_memmove8", "__aeabi_memmove4",
                 "__aeabi_memmove", "__aeabi_memset8",
                 "__aeabi_memset4", "__aeabi_memset",
                 "__aeabi_memclr8", "__aeabi_memclr4",
                 "__aeabi_memclr", "gpu_print_text", "gpu_reset",
                 "gpu_blank", "gpu_swap_buf", "gpu_send_buf",
                 "gpu_display_buf", "gpu_wait_for_next_ready",
                 "gpu_show_startup", "gpu_adjust_brightness",
                 "gpu_draw_sdcard", "gpu_update_palette",
                 "sdcard_init", "sdcard_calculate_crc7",
                 "sdcard_calculate_crc16", "sdcard_request_csd",
                 "sdcard_calculate_size_csdv2",
                 "sdcard_calculate_size_csdv1",
                 "sdcard_calculate_size", "sdcard_set_spi_baudrate",
                 "sdcard_establish_spi", "sdcard_set_spi_baudrate",
                 "sdcard_args_send_if_cond", "sdcard_select",
                 "sdcard_release", "sdcard_transceive", "sdcard_read",
                 "sdcard_read_buf", "sdcard_read_block",
                 "sdcard_send_block", "sdcard_transceive",
                 "sdcard_send_blocking",
                 "sdcard_send_command_blocking",
                 "sdcard_send_app_command_blocking",
                 "sdcard_boot_sequence", "sdcard_go_idle",
                 "sdcard_init", "sdcard_request_csd",
                 "sdcard_request_ocr", "sdcard_init_peripheral",
                 "sdcard_read_sector_partially", "sdcard_read_sector",
                 "sdcard_write_sector",
                 "sdcard_read_sector_partially", "sdcard_on_insert",
                 "sdcard_on_eject", "rng_init", "rng_u32", "atan",
                 "cos", "sin", "tan", "tanh", "frexp", "modf", "ceil",
                 "fabs", "floor", "acos", "asin", "atan2", "cosh",
                 "sinh", "exp", "ldexp", "log", "log10", "pow",
                 "sqrt", "fmod", "memset", "input_get_button",
                 "input_get_buttons", "input_is_available",
                 "input_get_availability", "input_change_freq",
                 "realloc"]

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
