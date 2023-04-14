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

#ifndef MES_GPU_H
#define MES_GPU_H

#include "font.h"
#include "gpu_internal.h"
#include <stdint.h>

/**
 * @param red: 0b000 - 0b111
 * @param green: 0b000 - 0b111
 * @param blue: 0b000 - 0b111
 * Generates a port config given the 3 colors.
 */
#define COLOR(red, green, blue)                                                \
    (uint16_t)(((red & 0b100) >> 1) | ((red & 0b010) << 9) |                   \
               ((red & 0b001) << 11) | ((green & 0b100) << 10) |               \
               ((green & 0b010) << 12) | ((green & 0b001) << 14) |             \
               ((blue & 0b100) << 5) | ((blue & 0b010) << 7) |                 \
               ((blue & 0b001) << 9))

enum Buffer { FRONT_BUFFER = 0, BACK_BUFFER = 1 } __attribute__((__packed__));

typedef enum Buffer Buffer;

/// Will halt execution until GPU acknowledged the operation.
void gpu_block_until_ack(void);

/// Will halt execution until n frames have passed.
void gpu_block_frames(uint8_t n);

/// Prints text on the Monitor.
void gpu_print_text(Buffer buffer, uint8_t ox, uint8_t oy, uint8_t foreground,
                    uint8_t background, const char *text);

/// Will reset the GPU. Resetting everything as if someone would disconnect the
/// power.
void gpu_reset(void);

/// Will blank the buffer using `black_with`.
void gpu_blank(Buffer buffer, uint8_t blank_with);

/// Will swap front and back buffer on the gpu.
void gpu_swap_buf(void);

/// Will send a buffer to the gpu.
void gpu_send_buf(Buffer buffer, uint8_t xx, uint8_t yy, uint8_t ox, uint8_t oy,
                  void *pixels);

/// Will send a buffer to the back buffer of the gpu and then swap using 1
/// operation.
void gpu_display_buf(uint8_t xx, uint8_t yy, uint8_t ox, uint8_t oy,
                     void *pixels);

/// Will block until next GPU READY is recieved, useful for frame timing.
void gpu_wait_for_next_ready(void);

/// Will display a startup animation. Internal operation.
void gpu_show_startup(void);

/// Will display a brightness adjustment screen and fade it out. Internal operation.
void gpu_adjust_brightness(void);

/// Will draw a sd-card icon at x y. Internal operation.
void gpu_draw_sdcard(Buffer buffer, uint8_t x, uint8_t y);

/// Will update the palette.
void gpu_update_palette(const uint16_t *new_palette);

#endif // MES_GPU_H
