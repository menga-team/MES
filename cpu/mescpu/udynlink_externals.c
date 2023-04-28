#include "udynlink_externals.h"
#include "gpu.h"
#include "stdint.h"
#include <aeabi.h>
#include <input.h>
#include <input_internal.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/adc.h>
#include <malloc.h>
#include <mes.h>
#include <sdcard.h>
#include <stdio.h>
#include <string.h>
#include <timer.h>
#include <udynlink.h>
#include <math.h>
#include <rng.h>

const char *unknown_symbol;

void *udynlink_external_malloc(size_t size) { return malloc(size); }

void udynlink_external_free(void *p) { return free(p); }

void udynlink_external_vprintf(const char UNUSED *s, va_list UNUSED va) {}

uint32_t udynlink_external_resolve_symbol(const char *name) {
#include "bin/symbols.inc"
    unknown_symbol = name;
    return 0;
}
