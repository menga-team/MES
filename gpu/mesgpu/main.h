#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include <malloc.h>
#include "gpu.h"

#ifndef MES_MAIN_H
#define MES_MAIN_H

void cpu_communication_timeout(void);

void generic_error(void);

void invalid_operation(uint8_t* invalid_op);

void unexpected_data(enum Stage c_stage);

void setup_output(void);

void setup_video(void);

void dma_recieve(uint32_t adr, uint32_t len);

void dma_recieve_operation(void);

void start_communication(void);

void start_video(void);

void handle_operation(void);

void new_operation(void);

void new_data(void);

int main(void);

#endif