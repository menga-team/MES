//#include <stdint.h>
//#include <limits.h>
//#include <assert.h>
//#include <timer.h>
////#include <libopencm3/stm32/common/rcc_common_all.h>
//#include <libopencm3/stm32/rcc.h>
//#include <libopencm3/stm32/gpio.h>
//
//const uint16_t VGA_OUT_WIDTH = 640;
//const uint16_t VGA_OUT_HEIGHT = 480;
//const uint8_t VGA_OUT_REFRESH_RATE = 60;
//const uint16_t BUFFER_WIDTH = VGA_OUT_WIDTH / 4;
//const uint16_t BUFFER_HEIGHT = VGA_OUT_HEIGHT / 4;
//const uint8_t BPP = 3;
//const uint8_t PIXEL_MASK = (1 << BPP) - 1;
//
//uint8_t get_pixel(const void *buffer, uint16_t position) {
//        // 3 bytes = 8 pairs of 3bit pixels
//        uint32_t bytes = (*(uint32_t *) (buffer + (position / 8) * BPP)) /*& 0x00FFFFFF*/;
//        return (bytes >> ((position % 8) * BPP)) & PIXEL_MASK;
//}
//
//void set_pixel(void *buffer, uint16_t position, uint8_t data) {
//        uint32_t *bytes = (uint32_t *) (buffer + (position / 8) * BPP);
//        uint8_t shift = (position % 8) * BPP;
//        uint32_t mask = PIXEL_MASK << shift;
//        *bytes = (*bytes & ~mask) | (data & PIXEL_MASK) << shift;
//}
//
//void swap_buffers(uint32_t **front_buffer, uint32_t **back_buffer) {
//        uint32_t *swap = *front_buffer;
//        *front_buffer = *back_buffer;
//        *back_buffer = swap;
//}
//
//void init() {
//        // define standard color palette
//        // the bits per pixel (bpp) define how large the palette can be.
//        assert(BPP >= 3); // there are 8 standard colors, so the palette needs to be index-able with at least 3 bits.
//        uint8_t color_palette[1 << BPP]; // 2^BPP
//        color_palette[0b000] = 0b00000000; // black
//        color_palette[0b001] = 0b11111111; // white
//        color_palette[0b010] = 0b11100000; // red
//        color_palette[0b011] = 0b00011100; // green
//        color_palette[0b100] = 0b00000011; // blue
//        color_palette[0b101] = 0b11111100; // yellow
//        color_palette[0b110] = 0b11100011; // magenta
//        color_palette[0b111] = 0b00011111; // cyan?
//        uint32_t buffer_a[(BUFFER_WIDTH * BUFFER_HEIGHT * BPP) / (CHAR_BIT * sizeof(uint32_t))];
//        uint32_t buffer_b[(BUFFER_WIDTH * BUFFER_HEIGHT * BPP) / (CHAR_BIT * sizeof(uint32_t))];
//        uint32_t *front_buffer = buffer_a;
//        uint32_t *back_buffer = buffer_b;
//}
//
//int main(void) {
//        rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
//        rcc_periph_clock_enable(RCC_GPIOA);
//        gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO11);
//        gpio_set(GPIOC, GPIO13);
//        while (1);
//}

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

int main()
{
        rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE16_72MHZ]);
        rcc_periph_clock_enable(RCC_GPIOC);
        gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
        while (1) {
                gpio_toggle(GPIOC, GPIO13);
                for(int i = 0; i< 80000; i++)
                        __asm__("nop");
        }
}