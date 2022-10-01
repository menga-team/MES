#include <stdint.h>
#include <libopencm3/stm32/gpio.h>


#ifndef MES_MAIN_H
#define MES_MAIN_H

static void setup_clock(void);

static void setup_output(void);

static void start_video(void);

static void set_color(uint8_t color);

static void reset_color(void);

int main(void);

#endif