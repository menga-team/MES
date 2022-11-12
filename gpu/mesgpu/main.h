#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include <malloc.h>

#ifndef MES_MAIN_H
#define MES_MAIN_H

void invalid_operation(uint8_t* invalid_op);

void setup_output(void);

void setup_video(void);

//void dma_recieve(uint32_t adr, uint32_t len);

void dma_recieve_operation(void);

void start_communication(void);

void start_video(void);

int main(void);

#endif