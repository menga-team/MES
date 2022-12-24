#ifndef MES_GPU_H
#define MES_GPU_H

#include "gpu_internal.h"

/// Prints text on the Monitor.
void gpu_print_text(Buffer buffer, uint8_t ox, uint8_t oy, uint8_t foreground, uint8_t background, const char *text);

/// Will reset the GPU. Resetting everything as if someone would disconnect the power.
void gpu_reset(void);

void gpu_blank(Buffer buffer, uint8_t blank_with);

#endif //MES_GPU_H
