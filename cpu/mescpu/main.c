#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include "main.h"
#include "time.h"
#include "gpu.h"
#include "controller.h"
#include "sdcard.h"

int main(void) {
        rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
        clock_peripherals();
        configure_io();
        controller_configure_io();
        configure_systick();
        controller_setup_timers();
        gpu_initiate_communication();
        gpu_block_until_ready();
        sdcard_init_peripheral();
        while (true);
}

void clock_peripherals(void) {
        // only clocks the most basic peripherals
        rcc_periph_clock_enable(RCC_AFIO);
        rcc_periph_clock_enable(RCC_GPIOA);
        rcc_periph_clock_enable(RCC_GPIOB);
        rcc_periph_clock_enable(RCC_GPIOC);
}

void configure_io(void) {
        gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13); // in-built
        gpio_set(GPIOC, GPIO13); // clear in-built led.
}

void __attribute__((weak)) sdcard_on_insert(void) {
        // NOP
}

void __attribute__((weak)) sdcard_on_eject(void) {
        // NOP
}