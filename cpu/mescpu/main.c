#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include "main.h"
#include "time.h"

int main(void) {
        rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
        configure_io();
        configure_systick();
        setup_timers();
        while (true);
}

void configure_io(void) {
        rcc_periph_clock_enable(RCC_AFIO);
        rcc_periph_clock_enable(RCC_GPIOA);
        rcc_periph_clock_enable(RCC_GPIOB);
        rcc_periph_clock_enable(RCC_GPIOC);

        gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_TIM1_CH1);
        gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13); // in-built
        gpio_set(GPIOC, GPIO13); // clear in-built led.
}

void setup_timers(void) {
        /*
         * TIM1 generates a clock with 50% duty cycle on PIN A8.
         * 0-50   -> high
         * 51-101 -> low
         */
        rcc_periph_clock_enable(RCC_TIM1);
        rcc_periph_reset_pulse(RST_TIM1);
        timer_enable_break_main_output(TIM1);
        timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
        timer_set_prescaler(TIM1, (rcc_apb2_frequency / CONTROLLER_FREQ) - 1); // divides timer by value
        timer_set_period(TIM1, 100 + 1); // +1 for 50% duty cycle
        timer_disable_preload(TIM1);
        timer_continuous_mode(TIM1);
        timer_set_counter(TIM1, 0);
        timer_enable_oc_preload(TIM1, TIM_OC1);
        timer_enable_oc_output(TIM1, TIM_OC1);
        timer_set_oc_mode(TIM1, TIM_OC1, TIM_OCM_PWM1);
        timer_set_oc_value(TIM1, TIM_OC1, 50 + 1);
        timer_set_oc_polarity_high(TIM1, TIM_OC1);
        timer_set_counter(TIM1, 0);
        timer_set_oc_value(TIM1, TIM_OC2, 1); // OC2 serves as interrupt.
        nvic_enable_irq(NVIC_TIM1_CC_IRQ);
        nvic_set_priority(NVIC_TIM1_CC_IRQ, 0);
        timer_enable_irq(TIM1, TIM_DIER_CC2IE);
        timer_enable_counter(TIM1);
}

void tim1_cc_isr(void) {
        // gets executed at the rising edge.
        TIM_SR(TIM1) = 0x0000;
}