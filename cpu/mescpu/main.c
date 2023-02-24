#include "main.h"
#include "gpu.h"
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

const uint8_t hello_world[] __attribute__((aligned(4))) = {
    0x55, 0x44, 0x4c, 0x4d, 0x02, 0x00, 0x02, 0x00, 0x40, 0x00, 0x00, 0x00,
    0x58, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x30,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x4c, 0x00, 0x00, 0x00,
    0x29, 0x00, 0x00, 0x50, 0x2d, 0x00, 0x00, 0x00, 0x2f, 0x00, 0x00, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x67, 0x61, 0x6d, 0x65, 0x00, 0x73, 0x74, 0x61,
    0x72, 0x74, 0x00, 0x67, 0x70, 0x75, 0x5f, 0x70, 0x72, 0x69, 0x6e, 0x74,
    0x5f, 0x74, 0x65, 0x78, 0x74, 0x00, 0x00, 0x00, 0x37, 0xb5, 0x00, 0x24,
    0x07, 0x4b, 0x08, 0x4d, 0x59, 0xf8, 0x03, 0x30, 0x00, 0x94, 0x01, 0x93,
    0x20, 0x46, 0x06, 0x23, 0x30, 0x22, 0x21, 0x46, 0x59, 0xf8, 0x05, 0x50,
    0xa8, 0x47, 0x20, 0x46, 0x03, 0xb0, 0x30, 0xbd, 0x00, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x2d, 0xe9, 0x00, 0x42, 0x03, 0xb4, 0x05, 0x49,
    0x09, 0x68, 0x78, 0x46, 0x88, 0x47, 0x81, 0x46, 0x03, 0xbc, 0xff, 0xf7,
    0xdf, 0xff, 0xbd, 0xe8, 0x00, 0x82, 0x00, 0x00, 0x50, 0x01, 0x00, 0x08,
    0x4d, 0x41, 0x49, 0x4e, 0x5f, 0x47, 0x4c, 0x4f, 0x42, 0x41, 0x4c, 0x00};

uint8_t run_game(void *adr) {
    gpu_print_text(FRONT_BUFFER, 0, 0, 1, 0, "LOADING GAME...");
    udynlink_module_t *p_game;
    udynlink_error_t p_error;
    p_game =
        udynlink_load_module(adr, NULL, 0, UDYNLINK_LOAD_MODE_XIP, &p_error);
    if (p_error != UDYNLINK_OK) {
        gpu_print_text(FRONT_BUFFER, 0, 8, 2, 0, "ERROR LOADING GAME.");
        char *error_str = malloc(27);
        sprintf(error_str, "p_error: %i", p_error);
        gpu_print_text(FRONT_BUFFER, 0, 24, 1, 0, error_str);
        free(error_str);
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
    // controller_configure_io();
    timer_start();
    // controller_setup_timers();
    gpu_initiate_communication();
    gpu_sync();
    if ((uint32_t)&get_lot_base != 0x08000150) {
        invalid_location_get_lot_base((uint32_t)&get_lot_base);
    }
    sdcard_init_peripheral();
    gpu_blank(BACK_BUFFER, 0);
    gpu_swap_buf();

    run_game((void *)hello_world);

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
