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
        setup_video();
        start_video();
        while (1);
        return 0;
}

void setup_clock(void) {
        rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
}

void setup_output(void) {
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

void setup_video(void) {
        for (uint16_t i = 0; i < BUFFER_HEIGHT * BUFFER_WIDTH; i++)
                gpu_set_pixel(buffer_a, i, i % 8);
        for (uint16_t i = 0; i < BUFFER_HEIGHT * BUFFER_WIDTH; i++)
                gpu_set_pixel(buffer_b, i, i + (i / 3) % 8);
//        gpu_swap_buffers();
//        gpu_set_pixel(front_buffer, 0, 0b111);
//        gpu_set_pixel(front_buffer, 1, 0b111);
//        gpu_set_pixel(front_buffer, 2, 0b111);
//        gpu_set_pixel(front_buffer, 3, 0b111);
//        gpu_set_pixel(front_buffer, 4, 0b111);
//        gpu_set_pixel(front_buffer, 5, 0b111);
//        gpu_set_pixel(front_buffer, 6, 0b111);
//        gpu_set_pixel(front_buffer, 7, 0b111);
//        gpu_set_pixel(front_buffer, 15, 0b010);
//        gpu_set_pixel(front_buffer, 16, 0b111);
//        gpu_set_pixel(front_buffer, BUFFER_WIDTH - 1, 0b101);
//
//        gpu_set_pixel(front_buffer, BUFFER_WIDTH * 6 - 1, 0b010);
//        for (uint16_t i = 0; i < 8; i++)
//                gpu_set_pixel(front_buffer, 16 + i, i);
//
//        for (uint16_t i = 1; i < BUFFER_WIDTH; i++)
//                gpu_set_pixel(front_buffer, (i * BUFFER_WIDTH) - 17, i % 8);

//        uint32_t pxs =
//                *(uint32_t *) (
//                        (const void *) front_buffer
//                        + (buffer_line * (BUFFER_WIDTH / 8) * BUFFER_BPP)
//                        + (pxs_index * BUFFER_BPP)
//                );
}

void start_video(void) {
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
        timer_set_period(TIM3, V_WHOLE_FRAME_LINES - 1);
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

void set_color(uint8_t color) {
        uint16_t port = get_port_config_for_color(color);
        GPIO_ODR(GPIO_COLOR_PORT) = port;
}

void reset_color(void) {
        GPIO_BRR(GPIO_COLOR_PORT) = 0xffff;
}

void __attribute__((optimize("O2"))) tim1_cc_isr(void) {
        if ((TIM_SR(TIM1) & TIM_SR_CC2IF) != 0) {
                TIM_SR(TIM1) = 0x0000;
                reset_color();
                // we have some spare time until the next interrupt ~970ns
                buffer_line = (TIM3_CNT - V_SYNC_PULSE_LINES - V_BACK_PORCH_LINES + 1) / 5;
                if (buffer_line >= BUFFER_HEIGHT) buffer_line = 0;
//                line = (const void *) front_buffer + (buffer_line * (BUFFER_WIDTH / 8) * BUFFER_BPP);
                // this is faster than using the pointer (or is it???)
                switch ((uint32_t) front_buffer) {
                        case BUFFER_A_ADDRESS:
                                line = (const void *) BUFFER_A_ADDRESS +
                                       (buffer_line * (BUFFER_WIDTH / 8) * BUFFER_BPP);
                                break;
                        case BUFFER_B_ADDRESS:
                                line = (const void *) BUFFER_B_ADDRESS +
                                       (buffer_line * (BUFFER_WIDTH / 8) * BUFFER_BPP);
                }
        } else {
                TIM_SR(TIM1) = 0x0000;
                if (buffer_line % 2 == 0)
                        return;
//                for (uint8_t pxs_index = 0; pxs_index < (uint8_t) (BUFFER_WIDTH / 8); ++pxs_index) {
//                        uint32_t pxs =
//                                *(uint32_t *) (
//                                        (const void *) front_buffer
//                                        + (buffer_line * (BUFFER_WIDTH / 8) * BUFFER_BPP)
//                                        + (pxs_index * BUFFER_BPP)
//                                );
                // TODO: macro?
                uint32_t pxs = *(uint32_t *) (line + (0 * BUFFER_BPP));
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (0 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (1 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (2 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (3 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (4 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (5 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (6 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (7 * BUFFER_BPP)) & PIXEL_MASK];
                pxs = *(uint32_t *) (line + (1 * BUFFER_BPP));
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (0 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (1 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (2 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (3 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (4 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (5 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (6 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (7 * BUFFER_BPP)) & PIXEL_MASK];
                pxs = *(uint32_t *) (line + (2 * BUFFER_BPP));
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (0 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (1 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (2 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (3 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (4 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (5 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (6 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (7 * BUFFER_BPP)) & PIXEL_MASK];
                pxs = *(uint32_t *) (line + (3 * BUFFER_BPP));
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (0 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (1 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (2 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (3 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (4 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (5 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (6 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (7 * BUFFER_BPP)) & PIXEL_MASK];
                pxs = *(uint32_t *) (line + (4 * BUFFER_BPP));
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (0 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (1 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (2 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (3 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (4 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (5 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (6 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (7 * BUFFER_BPP)) & PIXEL_MASK];
                pxs = *(uint32_t *) (line + (5 * BUFFER_BPP));
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (0 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (1 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (2 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (3 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (4 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (5 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (6 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (7 * BUFFER_BPP)) & PIXEL_MASK];
                pxs = *(uint32_t *) (line + (6 * BUFFER_BPP));
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (0 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (1 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (2 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (3 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (4 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (5 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (6 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (7 * BUFFER_BPP)) & PIXEL_MASK];
                pxs = *(uint32_t *) (line + (7 * BUFFER_BPP));
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (0 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (1 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (2 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (3 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (4 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (5 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (6 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (7 * BUFFER_BPP)) & PIXEL_MASK];
                pxs = *(uint32_t *) (line + (8 * BUFFER_BPP));
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (0 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (1 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (2 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (3 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (4 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (5 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (6 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (7 * BUFFER_BPP)) & PIXEL_MASK];
                pxs = *(uint32_t *) (line + (9 * BUFFER_BPP));
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (0 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (1 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (2 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (3 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (4 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (5 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (6 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (7 * BUFFER_BPP)) & PIXEL_MASK];
                pxs = *(uint32_t *) (line + (10 * BUFFER_BPP));
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (0 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (1 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (2 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (3 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (4 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (5 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (6 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (7 * BUFFER_BPP)) & PIXEL_MASK];
                pxs = *(uint32_t *) (line + (11 * BUFFER_BPP));
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (0 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (1 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (2 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (3 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (4 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (5 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (6 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (7 * BUFFER_BPP)) & PIXEL_MASK];
                pxs = *(uint32_t *) (line + (12 * BUFFER_BPP));
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (0 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (1 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (2 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (3 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (4 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (5 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (6 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (7 * BUFFER_BPP)) & PIXEL_MASK];
                pxs = *(uint32_t *) (line + (13 * BUFFER_BPP));
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (0 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (1 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (2 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (3 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (4 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (5 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (6 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (7 * BUFFER_BPP)) & PIXEL_MASK];
                pxs = *(uint32_t *) (line + (14 * BUFFER_BPP));
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (0 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (1 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (2 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (3 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (4 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (5 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (6 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (7 * BUFFER_BPP)) & PIXEL_MASK];
                pxs = *(uint32_t *) (line + (15 * BUFFER_BPP));
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (0 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (1 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (2 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (3 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (4 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (5 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (6 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (7 * BUFFER_BPP)) & PIXEL_MASK];
                pxs = *(uint32_t *) (line + (16 * BUFFER_BPP));
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (0 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (1 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (2 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (3 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (4 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (5 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (6 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (7 * BUFFER_BPP)) & PIXEL_MASK];
                pxs = *(uint32_t *) (line + (17 * BUFFER_BPP));
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (0 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (1 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (2 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (3 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (4 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (5 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (6 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (7 * BUFFER_BPP)) & PIXEL_MASK];
                pxs = *(uint32_t *) (line + (18 * BUFFER_BPP));
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (0 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (1 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (2 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (3 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (4 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (5 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (6 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (7 * BUFFER_BPP)) & PIXEL_MASK];
                pxs = *(uint32_t *) (line + (19 * BUFFER_BPP));
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (0 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (1 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (2 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (3 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (4 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (5 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (6 * BUFFER_BPP)) & PIXEL_MASK];
                GPIO_ODR(GPIO_COLOR_PORT) = color_palette[(pxs >> (7 * BUFFER_BPP)) & PIXEL_MASK];

//                }
        }
}