#include "main.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include "gpu.h"

uint16_t line[H_DISPLAY_PIXELS];

int main(void) {
        setup_clock();
        setup_output();
        start_video();
        gpu_init();
        for (uint16_t i = 0; i < BUFFER_HEIGHT * BUFFER_WIDTH; i++) {
                gpu_set_pixel(front_buffer, i, i % 8);
        }
        while (1);
        return 0;
}

static void setup_clock(void) {
        rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE16_72MHZ]);
}

static void setup_output(void) {
        rcc_periph_clock_enable(RCC_GPIOA);
        rcc_periph_clock_enable(RCC_GPIOB);
        rcc_periph_clock_enable(RCC_GPIOC);
        gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13); // built-in
        gpio_set(GPIOC, GPIO13);
        // A0: hsync (yellow cable)
        // A6: vsync (orange cable)
        gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_TIM2_CH1_ETR); // PA0
        gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_TIM3_CH1); // PA6
        gpio_set_mode(GPIO_COLOR_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                      RED_PIN_1 | RED_PIN_2 | RED_PIN_3 |
                      GREEN_PIN_1 | GREEN_PIN_2 | GREEN_PIN_3 |
                      BLUE_PIN_1 | BLUE_PIN_2);
        reset_color();
}

static void start_video(void) {
        // apb1 max clock speed: 36MHz -> TIM2, TIM3, TIM4
        // apb2 max clock speed: 72MHz -> TIM 1
        // TIM2 -> Horizontal sync
        rcc_periph_clock_enable(RCC_TIM2);
        rcc_periph_reset_pulse(RST_TIM2);
        timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
        // does the timer need to be that fast? maybe we're ok with a lower clock.
        timer_set_prescaler(TIM2,
                            (rcc_apb1_frequency / (PIXEL_CLOCK / 4))); // 36MHz / (36MHz / (36MHz / 4)) = 9MHz
        timer_set_period(TIM2, H_WHOLE_LINE_PIXELS / 4);
        timer_disable_preload(TIM2);
        timer_continuous_mode(TIM2);
        // We are cheating a bit here, we basically abuse the PWM hardware to generate our horizontal pulse, but because
        // the signal can only be aligned at one edge we need to send a longer signal before sending the pulse,
        // that also means that we are sending a longer signal than we actually need in the first iteration.
        // It is crucial to also adapt the VSync timer to this offset.
        // As a solution we start the timer with an already set counter, in this case H_BACK_PORCH_PIXELS.
        timer_set_oc_value(TIM2, TIM_OC1, H_SYNC_PULSE_PIXELS / 4);
        timer_set_counter(TIM2, 0);
        timer_enable_oc_preload(TIM2, TIM_OC1);
        timer_enable_oc_output(TIM2, TIM_OC1);
        timer_set_oc_mode(TIM2, TIM_OC1, TIM_OCM_PWM1);
        if (H_SYNC_POLARITY) timer_set_oc_polarity_high(TIM2, TIM_OC1);
        else timer_set_oc_polarity_low(TIM2, TIM_OC1);

        nvic_enable_irq(NVIC_TIM2_IRQ);
        nvic_set_priority(NVIC_TIM2_IRQ, 0);
        timer_set_oc_value(TIM2, TIM_OC2,
                           (H_SYNC_PULSE_PIXELS / 4 + H_BACK_PORCH_PIXELS / 4 + H_DISPLAY_PIXELS / 4));
        timer_enable_irq(TIM2, TIM_DIER_CC2IE);
        timer_set_oc_value(TIM2, TIM_OC3, (H_SYNC_PULSE_PIXELS / 4 + H_BACK_PORCH_PIXELS / 4));
        timer_enable_irq(TIM2, TIM_DIER_CC3IE);


        // TIM3 -> Vertical sync
        rcc_periph_clock_enable(RCC_TIM3);
        rcc_periph_reset_pulse(RST_TIM3);
        timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
        // we don't need to be any more precise than a line.
        timer_set_prescaler(TIM3, (rcc_apb1_frequency / (PIXEL_CLOCK /
                                                         H_WHOLE_LINE_PIXELS))); // 36MHz / (36MHz / (36MHz / 1024)) = 35.156kHz
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
        // TIM4 -> Pixel clock
        // enable both counters
        timer_enable_counter(TIM2);
        timer_enable_counter(TIM3);
}

static void set_color(uint8_t color) {
        uint16_t port = 0x0000;
        if ((color & 0b10000000) != 0) port |= RED_PIN_1;
        if ((color & 0b01000000) != 0) port |= RED_PIN_2;
        if ((color & 0b00100000) != 0) port |= RED_PIN_3;
        if ((color & 0b00010000) != 0) port |= GREEN_PIN_1;
        if ((color & 0b00001000) != 0) port |= GREEN_PIN_2;
        if ((color & 0b00000100) != 0) port |= GREEN_PIN_3;
        if ((color & 0b00000010) != 0) port |= BLUE_PIN_1;
        if ((color & 0b00000001) != 0) port |= BLUE_PIN_2;
        //gpio_port_write(GPIO_COLOR_PORT, port);
        GPIO_BSRR(GPIO_COLOR_PORT) = port;
}

static void reset_color(void) {
        GPIO_BRR(GPIO_COLOR_PORT) = 0xffff;

}

void tim2_isr(void) {
        if ((TIM_SR(TIM2) & TIM_SR_CC2IF) != 0) {
                TIM_SR(TIM2) = 0x0000;
                reset_color();
        } else {
                TIM_SR(TIM2) = 0x0000;
                GPIO_BSRR(GPIO_COLOR_PORT) = 0xffff;
                //__asm__ __volatile__("nop");
                reset_color();
                //GPIO_BRR(GPIO_COLOR_PORT) = 0xffff;
                //GPIO_BSRR(GPIO_COLOR_PORT) = 0xffff;
                //reset_color();
                //GPIO_BRR(GPIO_COLOR_PORT) = 0xffff;
        }
}