#ifndef MES_TIME_H
#define MES_TIME_H

#include "stdint.h"

void timer_start(void);

void timer_block_ms(uint32_t interval);

uint32_t timer_get_ms(void);

#endif //MES_TIME_H
