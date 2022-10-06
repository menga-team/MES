#include "main.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <mesmath.h>
#include "gpu.h"

int main(void) {
        setup_clock();
        setup_output();
        gpu_init();
        for (uint16_t i = 0; i < BUFFER_HEIGHT * BUFFER_WIDTH; i++) {
                gpu_set_pixel(front_buffer, i, 0b110);
        }
        gpu_set_pixel(front_buffer, 0, 0b111);
        gpu_set_pixel(front_buffer, 1, 0b111);
        gpu_set_pixel(front_buffer, 2, 0b111);
        gpu_set_pixel(front_buffer, 3, 0b111);
        gpu_set_pixel(front_buffer, 4, 0b111);
        gpu_set_pixel(front_buffer, 5, 0b111);
        gpu_set_pixel(front_buffer, 6, 0b111);
        gpu_set_pixel(front_buffer, 7, 0b111);

        start_video();
        while (1);
        return 0;
}

static void setup_clock(void) {
        rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
}

static void setup_output(void) {
        rcc_periph_clock_enable(RCC_AFIO);
        rcc_periph_clock_enable(RCC_GPIOA);
        rcc_periph_clock_enable(RCC_GPIOB);
        rcc_periph_clock_enable(RCC_GPIOC);
        gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13); // built-in
        gpio_set(GPIOC, GPIO13);
        // A8: hsync (yellow cable)
        // A6: vsync (orange cable)
        gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_TIM1_CH1); // PA8
        gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_TIM3_CH1); // PA6
        gpio_set_mode(GPIO_COLOR_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                      RED_PIN_1 | RED_PIN_2 | RED_PIN_3 |
                      GREEN_PIN_1 | GREEN_PIN_2 | GREEN_PIN_3 |
                      BLUE_PIN_1 | BLUE_PIN_2);
        reset_color();
}

static void start_video(void) {
        // timers are driven by a 72MHz clock
        // TIM1 -> Horizontal sync
        rcc_periph_clock_enable(RCC_TIM1);
        rcc_periph_reset_pulse(RST_TIM1);
        timer_enable_break_main_output(TIM1);
        timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
        // 0 = 1, so we need to subtract 1 to get the expected result.
        // tim1 is driven by rcc_apb2_frequency (72MHz)
        timer_set_prescaler(TIM1, uround(rcc_apb2_frequency / (PIXEL_CLOCK / (double) BUFFER_TO_VIDEO_RATIO)) - 1);
        timer_set_period(TIM1, H_WHOLE_LINE_PIXELS / 5); // 211 (211.2)
        timer_disable_preload(TIM1);
        timer_continuous_mode(TIM1);
        timer_set_counter(TIM1, 0);
        timer_enable_oc_preload(TIM1, TIM_OC1);
        timer_enable_oc_output(TIM1, TIM_OC1);
        timer_set_oc_mode(TIM1, TIM_OC1, TIM_OCM_PWM1);
        // This OC is responsible for the PWM signal, we are high until we reach OC1.
        timer_set_oc_value(TIM1, TIM_OC1, H_SYNC_PULSE_PIXELS / BUFFER_TO_VIDEO_RATIO); // 25 (25.6)
        if (H_SYNC_POLARITY) timer_set_oc_polarity_high(TIM1, TIM_OC1);
        else timer_set_oc_polarity_low(TIM1, TIM_OC1);
        nvic_enable_irq(NVIC_TIM1_CC_IRQ);
        nvic_set_priority(NVIC_TIM1_CC_IRQ, 0);
        // OC2 is responsible to reset the color, some monitors will base their black voltage level off the voltage
        // in the back porch, this means we can't send any colors in this period.
        timer_set_oc_value(TIM1, TIM_OC2, RESET_COLOR);
        timer_enable_irq(TIM1, TIM_DIER_CC2IE);
        // OC3 will let us know when we can start drawing.
        timer_set_oc_value(TIM1, TIM_OC3, START_DRAWING);
        timer_enable_irq(TIM1, TIM_DIER_CC3IE);

        // TIM3 -> Vertical sync
        rcc_periph_clock_enable(RCC_TIM3);
        rcc_periph_reset_pulse(RST_TIM3);
        timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
        // +6 is trial and error, because our hsync is not perfect we also need to change the vsync up a bit.
        timer_set_prescaler(TIM3, uround(rcc_apb2_frequency / (PIXEL_CLOCK / (double) H_WHOLE_LINE_PIXELS)) + 6);
        timer_set_period(TIM3, V_WHOLE_FRAME_LINES);
        timer_disable_preload(TIM3);
        timer_continuous_mode(TIM3);
        timer_set_oc_value(TIM3, TIM_OC1, V_SYNC_PULSE_LINES);
        timer_set_counter(TIM3, 0);
        timer_enable_oc_preload(TIM3, TIM_OC1);
        timer_enable_oc_output(TIM3, TIM_OC1);
        timer_set_oc_mode(TIM3, TIM_OC1, TIM_OCM_PWM1);
        if (V_SYNC_POLARITY) timer_set_oc_polarity_high(TIM3, TIM_OC1);
        else timer_set_oc_polarity_low(TIM3, TIM_OC1);

        // enable both counters
        timer_enable_counter(TIM3);
        timer_enable_counter(TIM1);
}

static void set_color(uint8_t color) {
        uint16_t port = get_port_config_for_color(color);
        GPIO_ODR(GPIO_COLOR_PORT) = port;
}

static void reset_color(void) {
        GPIO_BRR(GPIO_COLOR_PORT) = 0xffff;
}

uint16_t n_line = 0;

void __attribute__((optimize("O3"))) tim1_cc_isr(void) {
        if ((TIM_SR(TIM1) & TIM_SR_CC2IF) != 0) {
                TIM_SR(TIM1) = 0x0000;
                reset_color();
                // we have some spare time until the next interrupt ~970ns
                //prepare_scanline();
                n_line = (TIM3_CNT);
        } else {
                TIM_SR(TIM1) = 0x0000;
                for (uint8_t byte_index = 0; byte_index < (uint8_t) (BUFFER_WIDTH / 8); ++byte_index) {
                        uint32_t bytes = (*(uint32_t *) ((const void *) front_buffer + (n_line * (BUFFER_WIDTH / 8)) +
                                                         byte_index * BUFFER_BPP));
                        GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(bytes >> (0 * BUFFER_BPP)) & PIXEL_MASK];
                        GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(bytes >> (1 * BUFFER_BPP)) & PIXEL_MASK];
                        GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(bytes >> (2 * BUFFER_BPP)) & PIXEL_MASK];
                        GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(bytes >> (3 * BUFFER_BPP)) & PIXEL_MASK];
                        GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(bytes >> (4 * BUFFER_BPP)) & PIXEL_MASK];
                        GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(bytes >> (5 * BUFFER_BPP)) & PIXEL_MASK];
                        GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(bytes >> (6 * BUFFER_BPP)) & PIXEL_MASK];
                        GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(bytes >> (7 * BUFFER_BPP)) & PIXEL_MASK];
                }
        }
}