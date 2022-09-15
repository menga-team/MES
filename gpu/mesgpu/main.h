#include <stdint.h>

#ifndef MES_MAIN_H
#define MES_MAIN_H

static uint16_t overflow_counter = 0;

static void setup_clock(void);
static void setup_gpio(void);
static void setup_timers(void);

int main(void);

#endif