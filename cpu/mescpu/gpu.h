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

#include <stdint.h>
#include "font.h"
#include "gpu_internal.h"
#include "utils.h"

enum Buffer { FRONT_BUFFER = 0, BACK_BUFFER = 1 } __attribute__((__packed__));

typedef enum Buffer Buffer;

/// Will halt execution until GPU acknowledged the operation.
/// @related gpu_block_ack
void DEPRICATED gpu_block_until_ack(void);

/// Will wait for the next GPU READY Signal and because GPU READY gets
/// toggled every frame, it will block until the next frame is
/// displayed.
void gpu_block_frame(void);

/// Will wait until the last Operation has completed.
void gpu_block_ack(void);

/// Will halt execution until n frames have passed.
void gpu_block_frames(uint32_t n);

/// Prints text on the Monitor at Offsets ox and oy, foreground and
/// background parameters are indecies of the palette, meaning only
/// the first 3 bits are relevant.
void gpu_print_text(Buffer buffer, uint8_t ox, uint8_t oy, uint8_t foreground,
                    uint8_t background, const char *text);

/// Will reset the GPU. Resetting everything as if someone would disconnect the
/// power.
void gpu_reset(void);

/// Will blank the buffer using `black_with`.  Basically set every
/// byte in the video buffer to `blank_with`, subsequently only color
/// 0 or color 7 are possible.
void gpu_blank(Buffer buffer, uint8_t blank_with);

/// Will swap front and back buffer on the gpu.
void gpu_swap_buf(void);

/// Will send a buffer of size xx and yy to the gpu and insert it at
/// Offsets ox and oy.
void gpu_send_buf(Buffer buffer, uint8_t xx, uint8_t yy, uint8_t ox, uint8_t oy,
                  void *pixels);

/// Will send a buffer to the back buffer of the gpu and then swap using 1
/// operation. @related gpu_send_buf
void gpu_display_buf(uint8_t xx, uint8_t yy, uint8_t ox, uint8_t oy,
                     void *pixels);

/// Will block until next GPU READY is recieved, useful for frame timing.
/// @related gpu_block_frame
void DEPRICATED gpu_wait_for_next_ready(void);

/// Will display a startup animation. Internal operation.
void gpu_show_startup(void);

/// Will display a brightness adjustment screen and fade it out. Internal
/// operation.
void gpu_adjust_brightness(void);

/// Will draw a sd-card icon at x y. Internal operation.
void gpu_draw_sdcard(Buffer buffer, uint8_t x, uint8_t y);

/// Will draw a sd-card icon at x y. Internal operation.
void gpu_draw_controller(uint8_t bf, uint8_t n, uint8_t fg, uint8_t bg,
                         uint8_t x, uint8_t y);

/// Will update the palette. The palette consists of 8 uint16_t, the
/// colors are expected to be made with the COLOR macro which can be
/// found in mesgraphics.h.
void gpu_update_palette(const uint16_t *new_palette);

/// Will reset the palette to the standart palette.
void gpu_reset_palette(void);

#endif // MES_GPU_H
