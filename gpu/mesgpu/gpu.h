#include <stdint.h>
#include <limits.h>
#include <assert.h>

#ifndef MES_GPU_H
#define MES_GPU_H

// theoretically 800x600@56Hz would be the best resolution because its pixel clock is 36MHz,
// but unfortunately such resolutions are not widely supported, so we need to stick with 640x480@60.
// relevant timings:
// http://tinyvga.com/vga-timing/800x600@56Hz
// http://tinyvga.com/vga-timing/640x480@60Hz
#define H_DISPLAY_PIXELS 640
#define H_FRONT_PORCH_PIXELS 16
#define H_SYNC_PULSE_PIXELS 96
#define H_BACK_PORCH_PIXELS 48
#define V_DISPLAY_LINES 480
#define V_FRONT_PORCH_LINES 10
#define V_SYNC_PULSE_LINES 2
#define V_BACK_PORCH_LINES 33
#define H_WHOLE_LINE_PIXELS (H_DISPLAY_PIXELS + H_FRONT_PORCH_PIXELS + H_SYNC_PULSE_PIXELS + H_BACK_PORCH_PIXELS)
#define V_WHOLE_FRAME_LINES (V_DISPLAY_LINES + V_FRONT_PORCH_LINES + V_SYNC_PULSE_LINES + V_BACK_PORCH_LINES)
#define V_DISPLAY_LINES_PIXELS (V_DISPLAY_LINES * H_WHOLE_LINE_PIXELS)
#define V_FRONT_PORCH_LINES_PIXELS (V_FRONT_PORCH_LINES * H_WHOLE_LINE_PIXELS)
#define V_SYNC_PULSE_LINES_PIXELS (V_SYNC_PULSE_LINES * H_WHOLE_LINE_PIXELS)
#define V_BACK_PORCH_LINES_PIXELS (V_BACK_PORCH_LINES * H_WHOLE_LINE_PIXELS)
#define WHOLE_DISPLAY_PIXELS (V_WHOLE_FRAME_LINES*H_WHOLE_LINE_PIXELS)
#define PIXEL_CLOCK 25175000
// 0 -> idle high, active low
// 1 -> idle low, active high
#define H_SYNC_POLARITY 0
#define V_SYNC_POLARITY 0

// pixel buffer
#define VGA_OUT_REFRESH_RATE 60
#define BUFFER_WIDTH (H_DISPLAY_PIXELS / 4)
#define BUFFER_HEIGHT (V_DISPLAY_LINES / 4)
#define BUFFER_BPP 3
#define PIXEL_MASK ((1 << BUFFER_BPP) - 1)

// the bits per pixel (bpp) define how large the palette can be.
uint8_t color_palette[1 << BUFFER_BPP]; // 2^BUFFER_BPP

uint32_t buffer_a[(BUFFER_WIDTH * BUFFER_HEIGHT * BUFFER_BPP) / (CHAR_BIT * sizeof(uint32_t))];
uint32_t buffer_b[(BUFFER_WIDTH * BUFFER_HEIGHT * BUFFER_BPP) / (CHAR_BIT * sizeof(uint32_t))];
uint32_t *front_buffer, *back_buffer;

uint8_t gpu_get_pixel(const void *buffer, uint16_t position) {
        // 3 bytes = 8 pairs of 3bit pixels
        // TODO: make the project more scalable by implementing a generic function, together with an optimized one for
        //  3bpp, for getting/setting pixels. Also concerns `gpu_set_pixel`.
        uint32_t bytes = (*(uint32_t *) (buffer + (position / 8) * BUFFER_BPP)) /*& 0x00FFFFFF*/;
        return (bytes >> ((position % 8) * BUFFER_BPP)) & PIXEL_MASK;
}

void gpu_set_pixel(void *buffer, uint16_t position, uint8_t data) {
        uint32_t *bytes = (uint32_t *) (buffer + (position / 8) * BUFFER_BPP);
        uint8_t shift = (position % 8) * BUFFER_BPP;
        uint32_t mask = PIXEL_MASK << shift;
        *bytes = (*bytes & ~mask) | (data & PIXEL_MASK) << shift;
}

void gpu_swap_buffers(void) {
        uint32_t *swap = front_buffer;
        *front_buffer = *back_buffer;
        *back_buffer = *swap;
}

void gpu_init(void) {
        // there are 8 standard colors, so the palette needs to be index-able with at least 3 bits.
        assert(BUFFER_BPP >= 3);
        // define standard color palette.
        color_palette[0b000] = 0b00000000; // black
        color_palette[0b001] = 0b11111111; // white
        color_palette[0b010] = 0b11100000; // red
        color_palette[0b011] = 0b00011100; // green
        color_palette[0b100] = 0b00000011; // blue
        color_palette[0b101] = 0b11111100; // yellow
        color_palette[0b110] = 0b11100011; // magenta
        color_palette[0b111] = 0b00011111; // cyan?
        front_buffer = buffer_a;
        back_buffer = buffer_b;
}

#endif