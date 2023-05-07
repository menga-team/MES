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
#include "gpu_internal.h"
#include "input.h"
#include "input_internal.h"
#include "mes.h"
#include "mesgraphics.h"
#include "rng.h"
#include "sdcard.h"
#include "timer.h"
#include "udynlink.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/flash.h>
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
    udynlink_unload_module(p_game);
}

static void startup_animation(void) {
    uint32_t block_until = timer_get_ms() + 1000;
    while (timer_get_ms() < block_until) {
        if (input_get_button(CONTROLLER_1, BUTTON_A)) {
            return;
        }
    }
    gpu_block_frame();
    gpu_show_startup();
    gpu_block_frame();
    block_until = timer_get_ms() + 1000;
    while (timer_get_ms() < block_until) {
        if (input_get_button(CONTROLLER_1, BUTTON_A)) {
            return;
        }
    }
    gpu_block_frame();
    gpu_adjust_brightness();
    gpu_block_frame();
    block_until = timer_get_ms() + 500;
    while (timer_get_ms() < block_until) {
        if (input_get_button(CONTROLLER_1, BUTTON_A)) {
            return;
        }
    }
    gpu_block_frame();
}

static void check_controller(uint8_t controller) {
    gpu_blank(FRONT_BUFFER, 0);
    gpu_reset_palette();
    char *text = (char *)malloc(29);
    uint8_t fg;

    gpu_print_text(FRONT_BUFFER, 0, 112, 1, 0,
                   "PRESS " FONT_BUTTONA "+" FONT_BUTTONB " TO RETURN.");

    while (true) {
        volatile bool *buttons = input_get_buttons(controller);
        if (input_is_available(controller)) {
            sprintf(text, "CONTROLLER%i IS CONNECTED  ", controller + 1);
            fg = 3;
        } else {
            sprintf(text, "CONTROLLER%i IS DISCONNETED", controller + 1);
            fg = 2;
        }
        gpu_print_text(FRONT_BUFFER, 0, 8, fg, 0, text);
        gpu_block_ack();
        gpu_block_frame();
        for (uint8_t i = 0; i < 8; ++i) {
            if (buttons[i]) {
                sprintf(text, "%-3s PRESSED ", BUTTON_CHARACTERS[i]);
                fg = 3;
            } else {
                sprintf(text, "%-3s RELEASED", BUTTON_CHARACTERS[i]);
                fg = 2;
            }
            gpu_print_text(FRONT_BUFFER, 0, (i + 2) * 8, fg, 0, text);
            gpu_block_ack();
        }
        for (uint8_t i = 0; i < 4; ++i) {
            if (input_get_button(i, BUTTON_A) &&
                input_get_button(i, BUTTON_B)) {
                return;
            }
        }
    }
    free(text);
}

static void copy_game(uint8_t sectors) {
    if (SD_SECTOR_SIZE * 2 != FLASH_PAGE_SIZE) {
        error_with_message("calc", "");
    }

    flash_unlock();
    uint8_t *data = malloc(SD_SECTOR_SIZE * 2);
    uint32_t page_adr = FLASH_GAME_ADR;
    uint32_t flash_status = 0;
    for (uint32_t i = 0; i < sectors; i += 2) {
        // read from sd
        sdcard_read_sector(1 + i, data);
        sdcard_read_sector(2 + i, data + SD_SECTOR_SIZE);

        if ((FLASH_GAME_ADR - FLASH_BASE) >=
            (FLASH_PAGE_SIZE * (FLASH_PAGE_NUM_MAX + 1))) {
            error_with_message("out of bounds", "");
        }

        flash_erase_page(page_adr);
        flash_status = flash_get_status_flags();
        if (flash_status != FLASH_SR_EOP) {
            error_with_message("sr eop", "");
        }

        for (uint32_t word = 0; word < SD_SECTOR_SIZE * 2; word += 4) {
            flash_program_word(page_adr + word, *((uint32_t *)(data + word)));
            if (*((uint32_t *)(page_adr + word)) !=
                *((uint32_t *)(data + word))) {
                error_with_message("integrity failure", "");
            }
        }
        page_adr += FLASH_PAGE_SIZE;
    }
    free(data);
    flash_lock();
}

static void wait_for_sdcard(void) {
    const uint16_t original_color_palette[8] = {
        COLOR(0b000, 0b000, 0b000), COLOR(0b001, 0b001, 0b010),
        COLOR(0b010, 0b010, 0b100), COLOR(0b111, 0b001, 0b001),
        COLOR(0b011, 0b011, 0b011), COLOR(0b011, 0b011, 0b101),
        COLOR(0b111, 0b111, 0b111), COLOR(0b000, 0b000, 0b000),
    };
    GameImage *sector = malloc(512);
    bool data_loaded = false;
    gpu_update_palette(original_color_palette);
    int8_t offset = 0;
    bool elevate = true;
    uint8_t controller_selected = 0xff;
    uint32_t next_sd_event = timer_get_ms();
    bool pressed[4] = {false};
    while (true) {
        gpu_blank(BACK_BUFFER, 0x00);
        // sd card operation takes quite some time, we execute it on
        // its own frame.
        gpu_block_ack();
        gpu_block_frame();
        gpu_draw_sdcard(BACK_BUFFER, 62, 36 + offset);
        if (data_loaded) {
            // FIXME?: The title screen has more fps without sdcard.
            gpu_block_ack();
            gpu_block_frame();
            gpu_send_buf(BACK_BUFFER, 29, 26, 65, 42 + offset,
                         sector->icon.image);
        }
        const uint8_t controllers_y = 120 - 16 - 2;
        uint8_t controller_x = 160 - 2 - (18 * 4);
        for (uint8_t controller = 0; controller < 4; ++controller) {
            uint8_t fg = 3;
            uint8_t bg = 0;
            if (input_is_available(controller)) {
                if (data_loaded && input_get_button(controller, BUTTON_START)) {
                    // start the game.
                    goto end_waiting;
                }
                if (input_get_button(controller, BUTTON_SELECT)) {
                    if (!pressed[controller]) {
                        controller_selected = controller;
                    }
                    pressed[controller] = true;
                } else if (controller_selected != 0xff &&
                           input_get_button(controller, BUTTON_LEFT)) {
                    if (!pressed[controller]) {
                        controller_selected = (controller_selected - 1) & 0b11;
                    }
                    pressed[controller] = true;
                } else if (controller_selected != 0xff &&
                           input_get_button(controller, BUTTON_RIGHT)) {
                    if (!pressed[controller]) {
                        controller_selected = (controller_selected + 1) & 0b11;
                    }
                    pressed[controller] = true;
                } else if (input_get_button(controller, BUTTON_B)) {
                    if (!pressed[controller]) {
                        controller_selected = 0xff;
                    }
                    pressed[controller] = true;
                } else if (controller_selected != 0xff &&
                           input_get_button(controller, BUTTON_A)) {
                    if (!pressed[controller]) {
                        check_controller(controller_selected);
                        gpu_update_palette(original_color_palette);
                        controller_selected = 0xff;
                    }
                    pressed[controller] = true;
                } else {
                    pressed[controller] = false;
                }
                fg = 6;
            }

            if (controller == controller_selected) {
                SWAP(&fg, &bg);
            }

            gpu_draw_controller(BACK_BUFFER, controller, fg, bg, controller_x,
                                controllers_y);
            controller_x += 18;
        }

        if (data_loaded) {
            gpu_print_text(BACK_BUFFER, 2, 2, 6, 0, sector->name);
            gpu_print_text(BACK_BUFFER, 2, 10, 6, 0, sector->authors);
        }

        gpu_swap_buf();
        sdcard_poll();
        if (sdcard_available) {
            if (!data_loaded) {
                bool res = sdcard_init_peripheral();
                if (res) {
                    sdcard_read_sector(0, (uint8_t *)sector);
                    gpu_update_palette(sector->icon.palette);
                    data_loaded = true;
                }
            }
        } else {
            if (data_loaded) {
                gpu_update_palette(original_color_palette);
                data_loaded = false;
            }
        }

        if (timer_get_ms() >= next_sd_event) {
            bool delayed = false;

            if (elevate) {
                offset++;
            } else {
                offset--;
            }

            if (offset > 1) {
                elevate = false;
                next_sd_event = timer_get_ms() + 300;
                delayed = true;
            } else if (offset < -1) {
                elevate = true;
                next_sd_event = timer_get_ms() + 300;
                delayed = true;
            }

            next_sd_event = (delayed ? next_sd_event : timer_get_ms()) + 200;
        }

        gpu_block_ack();
        gpu_block_frame();
    }

end_waiting:
    copy_game(sector->sectors);
    free(sector);
}

static void reset(void) {
    uint32_t *aircr = (uint32_t *)(SCB_BASE + (3 * sizeof(uint32_t)));
    *aircr = ((0x5FA << 16) | (*aircr & (0x700)) | (1 << 2));
}

int main(void) {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
    clock_peripherals();
    configure_io();
    timer_start();
    input_setup();
    gpu_initiate_communication();

    // If gpu ready is already high, that means the cpu reset so we
    // don't need to do everything again.
    uint32_t wait_until = timer_get_ms() + 20;
    while (wait_until > timer_get_ms()) {
        if (gpio_port_read(GPU_READY_PORT) & GPU_READY) {
            if (hello_world[0] != 0x00) {
                gpu_block_ack();
                gpu_block_frame();
                goto run_flashed_game;
            } else {
                goto return_to_menu;
            }
        }
    }

    gpu_sync();
    gpu_reset_palette();
    if ((uint32_t)&get_lot_base != 0x08000150) {
        invalid_location_get_lot_base((uint32_t)&get_lot_base);
    }
    gpu_blank(BACK_BUFFER, 0);
    gpu_swap_buf();

    uint8_t exit;

    if (hello_world[0] != 0x00) {
        // run the flashed copy of the game instead.
    run_flashed_game:
        exit = run_game((void *)hello_world);
    } else {
        // normal startup, display a boot animation.
        startup_animation();
    return_to_menu:
        wait_for_sdcard();
        gpu_reset_palette();
        gpu_block_ack();
    restart_game:
        exit = run_game((void *)FLASH_GAME_ADR);
    }

    gpu_block_ack();

    switch (exit) {
    case CODE_EXIT:
        reset();
        break;
    case CODE_RESTART:
        goto restart_game;
    case CODE_FREEZEFRAME:
        break;
    default:
        gpu_print_text(FRONT_BUFFER, 0, 0, 1, 0, "GAME CLOSED UNEXPECTEDLY!");
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

// TODO finish function
void error_with_message(char *title, char *text) {
    unrecoverable_error();
    char *txt = malloc(27);
    sprintf(txt, "\xd6\xc4 %s-22 \xc4\xb7", title);
    gpu_print_text_blocking(FRONT_BUFFER, 2, 24, 1, 4, txt);
    free(txt);
    while (true)
        ;
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
    free(text_buf);
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
    sprintf(text_buf, "CFSR %08lx", *cfsr);
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
    sprintf(text_buf, "r0  %08lx r1 %08lx", r0, r1);
    gpu_print_text_blocking(FRONT_BUFFER, 8, line++ * 8, 1, 4, text_buf);
    sprintf(text_buf, "r2  %08lx r3 %08lx", r2, r3);
    gpu_print_text_blocking(FRONT_BUFFER, 8, line++ * 8, 1, 4, text_buf);
    sprintf(text_buf, "r12 %08lx lr %08lx", r12, lr);
    gpu_print_text_blocking(FRONT_BUFFER, 8, line++ * 8, 1, 4, text_buf);
    sprintf(text_buf, "psr %08lx pc %08lx", psr, pc);
    gpu_print_text_blocking(FRONT_BUFFER, 8, line++ * 8, 1, 4, text_buf);

    free(text_buf);
    __asm__ volatile("bkpt #01");
    while (true)
        ;
}

void enter_hard_fault_handler(void) __attribute__((aligned(4)))
__attribute__((naked));

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
