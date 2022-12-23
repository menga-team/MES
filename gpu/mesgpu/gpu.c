#include <memory.h>
#include "gpu.h"
#include "font.h"

// the bits per pixel (bpp) define how large the palette can be.
// colorid => port
uint16_t color_palette[1 << DRM_BUFFER_BPP]; // 2^BUFFER_BPP

uint8_t __attribute__((section (".buffer_a"))) buffer_a[
        (BUFFER_WIDTH * BUFFER_HEIGHT * BUFFER_BPP) / (CHAR_BIT * sizeof(uint8_t))];
uint8_t __attribute__((section (".buffer_b"))) buffer_b[
        (BUFFER_WIDTH * BUFFER_HEIGHT * BUFFER_BPP) / (CHAR_BIT * sizeof(uint8_t))];
uint8_t *front_buffer, *back_buffer;

uint8_t buffer_line = 0;
const void *line;

uint32_t pxs = 0;

uint8_t operation[OPERATION_LENGTH];
uint8_t operation_data[OPERATION_DATA_LENGTH];

volatile enum Stage processing_stage = UNINITIALIZED;

const char *stage_pretty_names[6] = {
        "Uninitialized",
        "Waiting for operation",
        "New operation",
        "Waiting for data",
        "New data",
        "Waiting for DMA"
};

volatile bool run = true;

uint8_t gpu_get_pixel(const void *buffer, uint16_t position) {
        // 3 bytes = 8 pairs of 3bit pixels
        uint32_t bytes = (*(uint32_t *) (buffer + (position / 8) * BUFFER_BPP)) /*& 0x00FFFFFF*/;
        return (bytes >> ((7 - (position % 8)) * BUFFER_BPP)) & PIXEL_MASK;
}

void gpu_set_pixel(void *buffer, uint16_t position, uint8_t data) {
        uint32_t *bytes = (uint32_t *) (buffer + (position / 8) * BUFFER_BPP);
        uint8_t shift = (7 - (position % 8)) * BUFFER_BPP;
        uint32_t mask = PIXEL_MASK << shift;
        *bytes = (*bytes & ~mask) | ((data & PIXEL_MASK) << shift);
}

void gpu_swap_buffers(void) {
        uint8_t *swap = front_buffer;
        front_buffer = back_buffer;
        back_buffer = swap;
}

void gpu_init(void) {
        // there are 8 standard colors, so the palette needs to be index-able with at least 3 bits.
        assert(BUFFER_BPP >= 3);
        // define standard color palette.
        color_palette[0] = COLOR(0b000, 0b000, 0b000); // black
        color_palette[1] = COLOR(0b111, 0b111, 0b111); // white
        color_palette[2] = COLOR(0b111, 0b000, 0b000); // red
        color_palette[3] = COLOR(0b000, 0b111, 0b000); // green
        color_palette[4] = COLOR(0b000, 0b000, 0b111); // blue
        color_palette[5] = COLOR(0b111, 0b111, 0b000); // yellow
        color_palette[6] = COLOR(0b111, 0b000, 0b111); // magenta
        color_palette[7] = COLOR(0b000, 0b111, 0b111); // cyan
        front_buffer = buffer_a;
        back_buffer = buffer_b;
}

void gpu_patch_font(void *patch, uint8_t start, uint8_t end) {
        memcpy(patch, &console_font_6x8[start * 8], (end - start) * 8);
}

void gpu_reset(void) {
        uint32_t *aircr = (uint32_t *) (SCB_BASE + (3 * sizeof(uint32_t)));
        *aircr = ((0x5FA << 16) | (*aircr & (0x700)) | (1 << 2));
}

void gpu_write(void *buffer, uint8_t x, uint8_t y, uint8_t fg, uint8_t bg, const char *text) {
        // TODO: Benchmark vs commit 199a54e34fa452cd73fd015bf033ed4b4a6d605a
        uint8_t i = 0;
        while (text[i] != '\0') {
                char to_print = text[i];
                for (int k = 0; k < 8; ++k) {
                        // Reading the code for `gpu_set_pixel` might help with understanding this one.
                        uint16_t position = ((y + k) * BUFFER_WIDTH + x);
                        uint32_t *bytes = (uint32_t *) (buffer + (position / 8) * BUFFER_BPP);
                        // This last shift might not be accurate, but it does not matter because it will get corrected.
                        // We just need a value that is garanteed not to be 0.
                        // If it is 0, we would be wasting cpu time because of the if in the loop below.
                        uint8_t last_shift = -1;
                        for (uint8_t j = 0; j < 6; ++j) {
                                uint8_t shift = (7 - ((position + j) % 8)) * BUFFER_BPP;
                                if (last_shift == 0) {
                                        // 0 is the last possible shift, meaning we reached the end.
                                        // We need to update our bytes because the next pixel won't fit.
                                        // Shift of 21 is the first pixel & shift of 0 is the last pixel.
                                        bytes = (uint32_t *) (buffer + ((position + j) / 8) * BUFFER_BPP);
                                }
                                *bytes = (*bytes & ~(PIXEL_MASK << shift)) |
                                         ((((console_font_6x8[8 * to_print + k] << j) & (1 << 7)) ? fg : bg) << shift);
                                last_shift = shift;
                        }
                }
                x += 6;
                i++;
        }
}
