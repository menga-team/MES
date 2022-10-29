#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include <malloc.h>

#ifndef MES_MAIN_H
#define MES_MAIN_H

uint16_t buffer_line = 0;
const uint32_t *(*frame_blocks);
const void *line;

void setup_clock(void);

void setup_output(void);

void setup_video(void);

void start_video(void);

void set_color(uint8_t color);

void reset_color(void);

int main(void);

#endif