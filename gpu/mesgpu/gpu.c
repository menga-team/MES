#include "gpu.h"
#include "font.h"

// the bits per pixel (bpp) define how large the palette can be.
// colorid => port
uint16_t color_palette[1 << BUFFER_BPP]; // 2^BUFFER_BPP

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

uint16_t get_port_config_for_color(uint8_t red, uint8_t green, uint8_t blue) {
        // TODO: maybe macro?
        uint16_t port = 0x0000;
        if (red & 0b100) port |= RED_MSP;
        if (red & 0b010) port |= RED_MMSP;
        if (red & 0b001) port |= RED_LSP;
        if (green & 0b100) port |= GREEN_MSP;
        if (green & 0b010) port |= GREEN_MMSP;
        if (green & 0b001) port |= GREEN_LSP;
        if (blue & 0b100) port |= BLUE_MSP;
        if (blue & 0b010) port |= BLUE_MMSP;
        if (blue & 0b001) port |= BLUE_LSP;
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
        color_palette[0] = get_port_config_for_color(0b000, 0b000, 0b000); // black
        color_palette[1] = get_port_config_for_color(0b111, 0b111, 0b111); // white
        color_palette[2] = get_port_config_for_color(0b111, 0b000, 0b000); // red
        color_palette[3] = get_port_config_for_color(0b000, 0b111, 0b000); // green
        color_palette[4] = get_port_config_for_color(0b000, 0b000, 0b111); // blue
        color_palette[5] = get_port_config_for_color(0b111, 0b111, 0b000); // yellow
        color_palette[6] = get_port_config_for_color(0b111, 0b000, 0b111); // magenta
        color_palette[7] = get_port_config_for_color(0b000, 0b111, 0b111); // cyan
        front_buffer = buffer_a;
        back_buffer = buffer_b;
}

void write(uint8_t x, uint8_t y, uint8_t fg, uint8_t bg, const char *text) {
        // TODO: Optimize lol
        uint8_t i = 0;
        while (text[i] != '\0') {
                char to_print = text[i];
                for (uint8_t j = 0; j < 6; ++j) {
                        for (int k = 0; k < 8; ++k) {
                                gpu_set_pixel(
                                        front_buffer,
                                        (y + k) * BUFFER_WIDTH + x + j,
                                        ((console_font_6x8[8 * to_print + k] << j) & (1 << 7)) ? fg : bg);
                        }
                }
                x += 6;
                i++;
        }
}
