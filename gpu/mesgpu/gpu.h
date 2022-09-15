#include <stdint.h>
#include <limits.h>
#include <assert.h>

#ifndef MES_GPU_H
#define MES_GPU_H

// pixel buffer
#define VGA_OUT_WIDTH 640
#define VGA_OUT_HEIGHT 480
#define VGA_OUT_REFRESH_RATE 60
#define BUFFER_WIDTH VGA_OUT_WIDTH / 4
#define BUFFER_HEIGHT VGA_OUT_HEIGHT / 4
#define BUFFER_BPP 3
#define PIXEL_MASK (1 << BUFFER_BPP) - 1

// timing
#define H_DISPLAY_PIXELS 640
#define H_FRONT_PORCH_PIXELS 16
#define H_SYNC_PULSE_PIXELS 96
#define H_BACK_PORCH_PIXELS 48
#define V_DISPLAY_LINES 480
#define V_FRONT_PORCH_LINES 10
#define V_SYNC_PULSE_LINES 2
#define V_BACK_PORCH_LINES 33
#define H_WHOLE_LINE_PIXELS H_DISPLAY_PIXELS + H_FRONT_PORCH_PIXELS + H_SYNC_PULSE_PIXELS + H_BACK_PORCH_PIXELS
#define V_WHOLE_FRAME_LINES V_DISPLAY_LINES + V_FRONT_PORCH_LINES + V_SYNC_PULSE_LINES + V_BACK_PORCH_LINES

uint8_t get_pixel(const void *buffer, uint16_t position) {
        // 3 bytes = 8 pairs of 3bit pixels
        uint32_t bytes = (*(uint32_t *) (buffer + (position / 8) * BUFFER_BPP)) /*& 0x00FFFFFF*/;
        return (bytes >> ((position % 8) * BUFFER_BPP)) & PIXEL_MASK;
}

void set_pixel(void *buffer, uint16_t position, uint8_t data) {
        uint32_t *bytes = (uint32_t *) (buffer + (position / 8) * BUFFER_BPP);
        uint8_t shift = (position % 8) * BUFFER_BPP;
        uint32_t mask = PIXEL_MASK << shift;
        *bytes = (*bytes & ~mask) | (data & PIXEL_MASK) << shift;
}

void swap_buffers(uint32_t **front_buffer, uint32_t **back_buffer) {
        uint32_t *swap = *front_buffer;
        *front_buffer = *back_buffer;
        *back_buffer = swap;
}

void init() {
        // define standard color palette
        // the bits per pixel (bpp) define how large the palette can be.
        assert(BUFFER_BPP >=
               3); // there are 8 standard colors, so the palette needs to be index-able with at least 3 bits.
        uint8_t color_palette[1 << BUFFER_BPP]; // 2^BUFFER_BPP
        color_palette[0b000] = 0b00000000; // black
        color_palette[0b001] = 0b11111111; // white
        color_palette[0b010] = 0b11100000; // red
        color_palette[0b011] = 0b00011100; // green
        color_palette[0b100] = 0b00000011; // blue
        color_palette[0b101] = 0b11111100; // yellow
        color_palette[0b110] = 0b11100011; // magenta
        color_palette[0b111] = 0b00011111; // cyan?
        uint32_t buffer_a[(BUFFER_WIDTH * BUFFER_HEIGHT * BUFFER_BPP) / (CHAR_BIT * sizeof(uint32_t))];
        uint32_t buffer_b[(BUFFER_WIDTH * BUFFER_HEIGHT * BUFFER_BPP) / (CHAR_BIT * sizeof(uint32_t))];
        uint32_t *front_buffer = buffer_a;
        uint32_t *back_buffer = buffer_b;
}

#endif