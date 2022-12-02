#ifndef MES_GPU_H
#define MES_GPU_H

#include "gpu_internal.h"

/// Prints text on the Monitor
void gpu_print_text(uint8_t ox, uint8_t oy, uint8_t foreground, uint8_t background, const char *text);

#endif //MES_GPU_H
