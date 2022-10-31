#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include <malloc.h>

#ifndef MES_MAIN_H
#define MES_MAIN_H

#define OPERATION_LENGTH 8
#define OPERATION_DATA_LENGTH 512

uint8_t buffer_line = 0;
const void *line;
uint32_t pxs = 0;
uint8_t operation[OPERATION_LENGTH];
uint8_t operation_data[OPERATION_DATA_LENGTH];

void setup_output(void);

void setup_video(void);

void start_communication(void);

void start_video(void);

int main(void);

#endif