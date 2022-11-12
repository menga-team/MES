#ifndef MES_TIME_H
#define MES_TIME_H

#include "stdint.h"

void configure_systick(void);

void block(uint32_t interval);

uint32_t millis_since_boot(void);

#endif //MES_TIME_H
