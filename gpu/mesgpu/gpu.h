#include <stdint.h>
#include <limits.h>
#include <assert.h>

#ifndef MES_GPU_H
#define MES_GPU_H

// relevant timings:
// http://tinyvga.com/vga-timing/800x600@60Hz
#define H_DISPLAY_PIXELS 800
#define H_FRONT_PORCH_PIXELS 40
#define H_SYNC_PULSE_PIXELS 128
#define H_BACK_PORCH_PIXELS 88

#define V_DISPLAY_LINES 600
#define V_FRONT_PORCH_LINES 1
#define V_SYNC_PULSE_LINES 4
#define V_BACK_PORCH_LINES 23

#define H_WHOLE_LINE_PIXELS (H_DISPLAY_PIXELS + H_FRONT_PORCH_PIXELS + H_SYNC_PULSE_PIXELS + H_BACK_PORCH_PIXELS)
#define V_WHOLE_FRAME_LINES (V_DISPLAY_LINES + V_FRONT_PORCH_LINES + V_SYNC_PULSE_LINES + V_BACK_PORCH_LINES)
#define V_DISPLAY_LINES_PIXELS (V_DISPLAY_LINES * H_WHOLE_LINE_PIXELS)
#define V_FRONT_PORCH_LINES_PIXELS (V_FRONT_PORCH_LINES * H_WHOLE_LINE_PIXELS)
#define V_SYNC_PULSE_LINES_PIXELS (V_SYNC_PULSE_LINES * H_WHOLE_LINE_PIXELS)
#define V_BACK_PORCH_LINES_PIXELS (V_BACK_PORCH_LINES * H_WHOLE_LINE_PIXELS)
#define WHOLE_DISPLAY_PIXELS (V_WHOLE_FRAME_LINES*H_WHOLE_LINE_PIXELS)
#define PIXEL_CLOCK 40000000
// 0 -> idle high, active low
// 1 -> idle low, active high
#define H_SYNC_POLARITY 1
#define V_SYNC_POLARITY 1

// pixel buffer
#define VGA_OUT_REFRESH_RATE 60
#define BUFFER_TO_VIDEO_RATIO 5
#define BUFFER_WIDTH (H_DISPLAY_PIXELS / BUFFER_TO_VIDEO_RATIO)
#define BUFFER_HEIGHT (V_DISPLAY_LINES / BUFFER_TO_VIDEO_RATIO)
#define BUFFER_BPP 3
#define PIXEL_MASK ((1 << BUFFER_BPP) - 1)

// colors
#define GPIO_COLOR_PORT GPIOB

#define RED_PIN_1 GPIO11 // most significant bit
#define RED_PIN_2 GPIO10
#define RED_PIN_3 GPIO1 // least significant bit

#define GREEN_PIN_1 GPIO12 // most significant bit
#define GREEN_PIN_2 GPIO13
#define GREEN_PIN_3 GPIO14 // least significant bit

#define BLUE_PIN_1 GPIO8 // most significant bit
#define BLUE_PIN_2 GPIO9 // least significant bit

#define GPU_READY_PIN GPIO15

// -11 because we need some time to get ready
#define PREPARE_DISPLAY ((H_SYNC_PULSE_PIXELS / 5 + H_BACK_PORCH_PIXELS / 5) - 11) // 42-11 (43.2)

// the bits per pixel (bpp) define how large the palette can be.
// colorid => port
uint16_t color_palette[1 << BUFFER_BPP]; // 2^BUFFER_BPP

#define BUFFER_A_ADDRESS 0x20000000
#define BUFFER_B_ADDRESS 0x20001c20

uint8_t __attribute__((section (".buffer_a"))) buffer_a[(BUFFER_WIDTH * BUFFER_HEIGHT * BUFFER_BPP) / (CHAR_BIT * sizeof(uint8_t))];
uint8_t __attribute__((section (".buffer_b"))) buffer_b[(BUFFER_WIDTH * BUFFER_HEIGHT * BUFFER_BPP) / (CHAR_BIT * sizeof(uint8_t))];
uint8_t *front_buffer, *back_buffer;

uint16_t get_port_config_for_color(uint8_t color) {
        uint16_t port = 0x0000;
        if ((color & 0b10000000) != 0) port |= RED_PIN_1;
        if ((color & 0b01000000) != 0) port |= RED_PIN_2;
        if ((color & 0b00100000) != 0) port |= RED_PIN_3;
        if ((color & 0b00010000) != 0) port |= GREEN_PIN_1;
        if ((color & 0b00001000) != 0) port |= GREEN_PIN_2;
        if ((color & 0b00000100) != 0) port |= GREEN_PIN_3;
        if ((color & 0b00000010) != 0) port |= BLUE_PIN_1;
        if ((color & 0b00000001) != 0) port |= BLUE_PIN_2;
        return port;
}

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
        color_palette[0b000] = get_port_config_for_color(0b00000000); // black
        color_palette[0b001] = get_port_config_for_color(0b11111111); // white
        color_palette[0b010] = get_port_config_for_color(0b11100000); // red
        color_palette[0b011] = get_port_config_for_color(0b00011100); // green
        color_palette[0b100] = get_port_config_for_color(0b00000011); // blue
        color_palette[0b101] = get_port_config_for_color(0b11111100); // yellow
        color_palette[0b110] = get_port_config_for_color(0b11100011); // magenta
        color_palette[0b111] = get_port_config_for_color(0b00011111); // cyan
        front_buffer = buffer_a;
        back_buffer = buffer_b;
}

#endif