#include "gpu.h"
#include "mes.h"
#include "mesgraphics.h"
#include "sdcard.h"
#include "timer.h"
#include "udynlink.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <malloc.h>
#include <math.h>
#include <memory.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

uint32_t __attribute__((section(".get_lot_base"))) (*const get_lot_base)(
    uint32_t) = &udynlink_get_lot_base;

const char *GAME_ENTRY = "start";
const uint32_t REVISION = 0;

// Used for testing a game without flashing a sdcard.
#include "dummy_game.h"

uint8_t run_game(void *adr) {
    gpu_print_text(FRONT_BUFFER, 0, 0, 1, 0, "LOADING GAME...");
    udynlink_module_t *p_game;
    udynlink_error_t p_error;
    p_game =
        udynlink_load_module(adr, NULL, 0, UDYNLINK_LOAD_MODE_XIP, &p_error);
    if (p_error != UDYNLINK_OK) {
        gpu_print_text(FRONT_BUFFER, 0, 8, 2, 0, "ERROR LOADING GAME.");
        char *error_str;
        switch (p_error) {
        case UDYNLINK_ERR_LOAD_INVALID_SIGN:
            error_str = "p_error: INVALID_SIGN";
            break;
        case UDYNLINK_ERR_LOAD_RAM_LEN_LOW:
            error_str = "p_error: RAM_LEN_LOW";
            break;
        case UDYNLINK_ERR_LOAD_OUT_OF_MEMORY:
            error_str = "p_error: OUT_OF_MEMORY";
            break;
        case UDYNLINK_ERR_LOAD_UNABLE_TO_XIP:
            error_str = "p_error: UNABLE_TO_XIP";
            break;
        case UDYNLINK_ERR_LOAD_NO_MORE_HANDLES:
            error_str = "p_error: NO_MORE_HANDLES";
            break;
        case UDYNLINK_ERR_LOAD_INVALID_MODE:
            error_str = "p_error: INVALID_MODE";
            break;
        case UDYNLINK_ERR_LOAD_BAD_RELOCATION_TABLE:
            error_str = "p_error: BAD_RELOC_TABLE";
            break;
        case UDYNLINK_ERR_LOAD_UNKNOWN_SYMBOL:
            error_str = "p_error: UNKNOWN_SYMBOL";
            // print the unknown symbol...
            gpu_print_text(FRONT_BUFFER, 0, 32, 1, 0, unknown_symbol);
            break;
        case UDYNLINK_ERR_LOAD_DUPLICATE_NAME:
            error_str = "p_error: DUPLICATE_NAME";
            break;
        default:
            error_str = malloc(27);
            sprintf(error_str, "p_error: %i", p_error);
            break;
        }
        gpu_print_text(FRONT_BUFFER, 0, 24, 1, 0, error_str);
        return -1;
    }

    gpu_print_text(FRONT_BUFFER, 0, 8, 1, 0, "STARTING GAME...");
    udynlink_sym_t sym;
    udynlink_lookup_symbol(p_game, GAME_ENTRY, &sym);
    uint8_t (*p_main)(void) = (uint8_t(*)(void))sym.val;

    p_main();
}

int main(void) {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
    clock_peripherals();
    configure_io();
    timer_start();
    gpu_initiate_communication();
    gpu_sync();
    if ((uint32_t)&get_lot_base != 0x08000150) {
        invalid_location_get_lot_base((uint32_t)&get_lot_base);
    }
    sdcard_init_peripheral();
    gpu_blank(BACK_BUFFER, 0);
    gpu_swap_buf();

    if (*hello_world != 0x00) {
        // run the flashed copy of the game instead.
        run_game((void *)hello_world);
    } else {
        // normal startup, display a boot animation
        timer_block_ms(1000); // wait for monitor
        gpu_wait_for_next_ready();
        gpu_show_startup();
        gpu_wait_for_next_ready();
		timer_block_ms(1000);
        gpu_wait_for_next_ready();
        gpu_adjust_brightness();
        gpu_wait_for_next_ready();
        timer_block_ms(500);
        gpu_wait_for_next_ready();
        gpu_print_text(FRONT_BUFFER, 0, 0, 1, 0, "DYN LOADING UNIMPLEMENTED!");
    }

    while (true)
        ;
}

void clock_peripherals(void) {
    // only clocks the most basic peripherals
    rcc_periph_clock_enable(RCC_AFIO);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
}

void configure_io(void) {
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                  GPIO13);   // in-built
    gpio_set(GPIOC, GPIO13); // clear in-built led.
}

void unrecoverable_error(void) {
    for (uint8_t i = 4; i < 11; ++i) {
        gpu_print_text_blocking(FRONT_BUFFER, 2, i * 8, 1, 4,
                                "\xba                        \xba");
    }
    gpu_print_text_blocking(
        FRONT_BUFFER, 2, 88, 1, 4,
        "\xc8\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd"
        "\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xbc");
}

void invalid_location_get_lot_base(uint32_t adr) {
    unrecoverable_error();
    gpu_print_text_blocking(
        FRONT_BUFFER, 2, 24, 1, 4,
        "\xd6\xc4\xc4\xc4 INVALID LOCATION \xc4\xc4\xc4\xb7");
    gpu_print_text_blocking(FRONT_BUFFER, 8, 40, 1, 4,
                            "The location of section");
    gpu_print_text_blocking(FRONT_BUFFER, 8, 48, 1, 4,
                            ".get_lot_base should be:");
    gpu_print_text_blocking(FRONT_BUFFER, 8, 56, 1, 4,
                            "0x08000150, but it was:");
    char *text_buf = malloc(27);
    sprintf(text_buf, "0x%08lx. Please", adr);
    gpu_print_text_blocking(FRONT_BUFFER, 8, 64, 1, 4, text_buf);
    gpu_print_text_blocking(FRONT_BUFFER, 8, 72, 1, 4,
                            "ensure that all code was");
    gpu_print_text_blocking(FRONT_BUFFER, 8, 80, 1, 4, "flashed correctly.");
    while (true)
        ;
}

void hard_fault_handler(void) {
    unrecoverable_error();
    gpu_print_text_blocking(FRONT_BUFFER, 2, 24, 1, 4,
                            "\xd6\xc4\xc4 H A R D  F A U L T \xc4\xc4\xb7");
    char *text_buf = malloc(27);
    uint16_t ufsr = *(uint16_t *)0xE000ED2A;
    sprintf(text_buf, "USFR:%04x \x1a", ufsr);
    gpu_print_text_blocking(FRONT_BUFFER, 8, 32, 1, 4, text_buf);
    uint8_t line = 4;
    if (USFR_IS_UNDEFSTR(ufsr))
        gpu_print_text_blocking(FRONT_BUFFER, 86, line++ * 8, 1, 4, "UNDEFSTR");
    if (USFR_IS_INVSTATE(ufsr))
        gpu_print_text_blocking(FRONT_BUFFER, 86, line++ * 8, 1, 4, "INVSTATE");
    if (USFR_IS_INVPC(ufsr))
        gpu_print_text_blocking(FRONT_BUFFER, 86, line++ * 8, 1, 4, "INVPC");
    if (USFR_IS_NOCP(ufsr))
        gpu_print_text_blocking(FRONT_BUFFER, 86, line++ * 8, 1, 4, "NOCP");
    if (USFR_IS_UNALIGNED(ufsr))
        gpu_print_text_blocking(FRONT_BUFFER, 86, line++ * 8, 1, 4,
                                "UNALIGNED");
    if (USFR_IS_DIVBYZERO(ufsr))
        gpu_print_text_blocking(FRONT_BUFFER, 86, line * 8, 1, 4, "DIVBYZERO");
    free(text_buf);
    while (true)
        ;
}
