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
