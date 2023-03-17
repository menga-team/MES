#include "udynlink_externals.h"
#include "gpu.h"
#include "stdint.h"
#include <malloc.h>
#include <string.h>
#include <sdcard.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/flash.h>
#include <stdio.h>
#include <udynlink.h>
#include <timer.h>
#include <controller.h>
#include <mes.h>

void *udynlink_external_malloc(size_t size) { return malloc(size); }

void udynlink_external_free(void *p) { return free(p); }

void udynlink_external_vprintf(const char *s, va_list va) {}

uint32_t udynlink_external_resolve_symbol(const char *name) {
    // TODO: This is just temporary for testing
    // gpu_print_text(FRONT_BUFFER, 0, 84, 2, 0, name);
    #include "bin/symbols.inc"
    return 0;
}
