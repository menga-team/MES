#include <stdint.h>
#include <libopencm3/stm32/gpio.h>

#ifndef MES_MAIN_H
#define MES_MAIN_H

uint16_t buffer_line = 0;
const void *line;
uint32_t scan_line[20];

void setup_clock(void);

void setup_output(void);

void setup_video(void);

void start_video(void);

void set_color(uint8_t color);

void reset_color(void);

int main(void);

#endif