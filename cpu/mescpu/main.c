#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include "main.h"
#include "time.h"
#include "controller.h"

int main(void) {
        rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
        configure_io();
        configure_systick();
        controller_setup_timers();
        while (true);
}

void configure_io(void) {
        rcc_periph_clock_enable(RCC_AFIO);
        rcc_periph_clock_enable(RCC_GPIOA);
        rcc_periph_clock_enable(RCC_GPIOB);
        rcc_periph_clock_enable(RCC_GPIOC);

        gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13); // in-built
        gpio_set(GPIOC, GPIO13); // clear in-built led.
        controller_configure_io();
}