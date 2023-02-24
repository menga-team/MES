#include "timer.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>

volatile uint32_t system_ms = 0;

void timer_start(void) {
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_set_reload(rcc_ahb_frequency / 1000); // overflows at 1kHz = 1ms
    systick_interrupt_enable();
    systick_counter_enable();
}

void timer_block_ms(uint32_t interval) {
    uint32_t block_until = system_ms + interval;
    while (system_ms < block_until)
        ;
}

uint32_t timer_get_ms(void) { return system_ms; }

void sys_tick_handler(void) { system_ms++; }
