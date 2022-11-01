#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>
#include "time.h"

volatile uint32_t system_millis = 0;

void configure_systick(void) {
        systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
        systick_set_reload(rcc_ahb_frequency / 1000); // overflows at 1kHz = 1ms
        systick_interrupt_enable();
        systick_counter_enable();
}

void block(uint32_t interval) {
        uint32_t block_until = system_millis + interval;
        while (system_millis < block_until);
}

uint32_t millis_since_boot(void) {
        return system_millis;
}

void sys_tick_handler(void) {
        system_millis++;
}
