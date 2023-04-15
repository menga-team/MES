/* This file is part of MES.
 *
 * MES is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MES is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warrantyp of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MES. If not, see <https://www.gnu.org/licenses/>.
 */

#include "gpu.h"
#include "mes.h"
#include "mesgraphics.h"
#include "rng.h"
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

const uint32_t REVISION =
#include "../../REVISION"
    ;

// Used for testing a game without flashing a sdcard.
#include "dummy_game.h"

uint8_t run_game(void *adr) {
    gpu_print_text(FRONT_BUFFER, 0, 0, 1, 0, "LOADING GAME...");
    udynlink_module_t *p_game;
    udynlink_error_t p_error;
    p_game =
        udynlink_load_module(adr, NULL, 0, UDYNLINK_LOAD_MODE_XIP, &p_error);
    if (p_error != UDYNLINK_OK) {
        gpu_print_text(FRONT_BUFFER, 0, 8, 2, 0, "ERROR LOADING GAME:");
        gpu_print_text(FRONT_BUFFER, 0, 16, 1, 0, error_codes[p_error]);
        if (p_error == UDYNLINK_ERR_LOAD_UNKNOWN_SYMBOL) {
            gpu_print_text(FRONT_BUFFER, 0, 24, 1, 0, unknown_symbol);
        }
        return -1;
    }

    gpu_print_text(FRONT_BUFFER, 0, 8, 1, 0, "STARTING GAME...");
    udynlink_sym_t sym;
    if (udynlink_lookup_symbol(p_game, GAME_ENTRY, &sym) != NULL) {
        uint8_t (*p_main)(void) = (uint8_t(*)(void))sym.val;
        return p_main();
    } else {
        gpu_print_text(FRONT_BUFFER, 0, 8, 2, 0, "UNABLE TO FIND ENTRYPOINT:");
        gpu_print_text(FRONT_BUFFER, 0, 16, 1, 0, GAME_ENTRY);
        return -1;
    }
}

static void startup_animation(void) {
    timer_block_ms(1000);
    gpu_wait_for_next_ready();
    gpu_show_startup();
    gpu_wait_for_next_ready();
    timer_block_ms(1000);
    gpu_wait_for_next_ready();
    gpu_adjust_brightness();
    gpu_wait_for_next_ready();
    timer_block_ms(500);
    gpu_wait_for_next_ready();
}

static void wait_for_sdcard(void) {
    uint16_t color_palette[8] = {
        COLOR(0b000, 0b000, 0b000), COLOR(0b001, 0b001, 0b010),
        COLOR(0b010, 0b010, 0b100), COLOR(0b111, 0b001, 0b001),
        COLOR(0b011, 0b011, 0b011), COLOR(0b011, 0b011, 0b101),
        COLOR(0b111, 0b111, 0b111), COLOR(0b000, 0b000, 0b000),
    };
    gpu_wait_for_next_ready();
    gpu_update_palette(color_palette);
    bool game_ready = false;
    int8_t offset = 0;
    bool elevate = true;
    while (!game_ready) {
        gpu_wait_for_next_ready();
        gpu_blank(BACK_BUFFER, 0x00);
        gpu_draw_sdcard(BACK_BUFFER, 62, 36 + offset);
        gpu_swap_buf();

        if (offset > 1) {
            elevate = false;
            timer_block_ms(300);
        } else if (offset < -1) {
            elevate = true;
            timer_block_ms(300);
        }

        if (elevate) {
            offset++;
        } else {
            offset--;
        }

        timer_block_ms(200);
    }
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
        // normal startup, display a boot animation.
        startup_animation();
        wait_for_sdcard();
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

void handle_hard_fault(uint32_t *stack_frame) {
    unrecoverable_error();
    gpu_print_text_blocking(FRONT_BUFFER, 2, 24, 1, 4,
                            "\xd6\xc4\xc4 H A R D  F A U L T \xc4\xc4\xb7");
    char *text_buf = malloc(27);
    volatile uint32_t *cfsr = (volatile uint32_t *)0xE000ED28;
    uint16_t ufsr = (uint16_t)(*cfsr >> 16);
    uint8_t bfsr = (uint8_t)(*cfsr >> 8);
    uint8_t mmfsr = (uint8_t)(*cfsr & 0xff);
    sprintf(text_buf, "CFSR %08x", *cfsr);
    gpu_print_text_blocking(FRONT_BUFFER, 8, 32, 1, 4, text_buf);
    uint8_t line = 4;
    if (USFR_IS_UNDEFSTR(ufsr))
        gpu_print_text_blocking(FRONT_BUFFER, 94, line++ * 8, 1, 4, "UNDEFSTR");
    if (USFR_IS_INVSTATE(ufsr))
        gpu_print_text_blocking(FRONT_BUFFER, 94, line++ * 8, 1, 4, "INVSTATE");
    if (USFR_IS_INVPC(ufsr))
        gpu_print_text_blocking(FRONT_BUFFER, 94, line++ * 8, 1, 4, "INVPC");
    if (USFR_IS_NOCP(ufsr))
        gpu_print_text_blocking(FRONT_BUFFER, 94, line++ * 8, 1, 4, "NOCP");
    if (USFR_IS_UNALIGNED(ufsr))
        gpu_print_text_blocking(FRONT_BUFFER, 94, line++ * 8, 1, 4,
                                "UNALIGNED");
    if (USFR_IS_DIVBYZERO(ufsr))
        gpu_print_text_blocking(FRONT_BUFFER, 94, line++ * 8, 1, 4,
                                "DIVBYZERO");
    if (MMFSR_IS_IACCVIOL(mmfsr))
        gpu_print_text_blocking(FRONT_BUFFER, 94, line++ * 8, 1, 4, "IACCVIOL");
    if (MMFSR_IS_DACCVIOL(mmfsr))
        gpu_print_text_blocking(FRONT_BUFFER, 94, line++ * 8, 1, 4, "DACCVIOL");
    if (MMFSR_IS_MUNSTKERR(mmfsr))
        gpu_print_text_blocking(FRONT_BUFFER, 94, line++ * 8, 1, 4,
                                "MUNSTKERR");
    if (MMFSR_IS_MSTKERR(mmfsr))
        gpu_print_text_blocking(FRONT_BUFFER, 94, line++ * 8, 1, 4, "MSTKERR");
    if (MMFSR_IS_MLSPERR(mmfsr))
        gpu_print_text_blocking(FRONT_BUFFER, 94, line++ * 8, 1, 4, "MLSPERR");
    if (MMFSR_IS_MMARVALID(mmfsr))
        gpu_print_text_blocking(FRONT_BUFFER, 94, line++ * 8, 1, 4,
                                "MMARVALID");

    uint32_t r0 = stack_frame[0];
    uint32_t r1 = stack_frame[1];
    uint32_t r2 = stack_frame[2];
    uint32_t r3 = stack_frame[3];
    uint32_t r12 = stack_frame[4];
    uint32_t lr = stack_frame[5];
    uint32_t pc = stack_frame[6];
    uint32_t psr = stack_frame[7];
    line = 7;
    sprintf(text_buf, "r0  %08x r1 %08x", r0, r1);
    gpu_print_text_blocking(FRONT_BUFFER, 8, line++ * 8, 1, 4, text_buf);
    sprintf(text_buf, "r2  %08x r3 %08x", r2, r3);
    gpu_print_text_blocking(FRONT_BUFFER, 8, line++ * 8, 1, 4, text_buf);
    sprintf(text_buf, "r12 %08x lr %08x", r12, lr);
    gpu_print_text_blocking(FRONT_BUFFER, 8, line++ * 8, 1, 4, text_buf);
    sprintf(text_buf, "psr %08x pc %08x", psr, pc);
    gpu_print_text_blocking(FRONT_BUFFER, 8, line++ * 8, 1, 4, text_buf);

    free(text_buf);
    __asm__ volatile("bkpt #01");
    while (true)
        ;
}

void enter_hard_fault_handler(void) __attribute__((naked));

void enter_hard_fault_handler(void) {
    __asm__ volatile("tst lr, #4				\n"
                     "ite eq					\n"
                     "mrseq r0, msp				\n"
                     "mrsne r0, psp				\n"
                     "ldr r1, [r0, #24]				\n"
                     "ldr r2, __hf_handler			\n"
                     "bx r2					\n"

                     "__hf_handler: .word handle_hard_fault	\n");
}

void hard_fault_handler(void) { enter_hard_fault_handler(); }
