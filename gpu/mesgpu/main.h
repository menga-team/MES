#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include <malloc.h>

#ifndef MES_MAIN_H
#define MES_MAIN_H

void invalid_operation(uint8_t* invalid_op);

void setup_output(void);

void setup_video(void);

void start_communication(void);

void start_video(void);

int main(void);

#endif