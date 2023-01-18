#ifndef MES_GPU_H
#define MES_GPU_H

#include "gpu_internal.h"

enum Buffer {
    FRONT_BUFFER = 0, BACK_BUFFER = 1
} __attribute__ ((__packed__));

typedef enum Buffer Buffer;

/// Prints text on the Monitor.
void gpu_print_text(Buffer buffer, uint8_t ox, uint8_t oy, uint8_t foreground, uint8_t background, const char *text);

/// Will reset the GPU. Resetting everything as if someone would disconnect the power.
void gpu_reset(void);

void gpu_blank(Buffer buffer, uint8_t blank_with);

void gpu_send_buf(Buffer buffer, uint8_t xx, uint8_t yy, uint8_t ox, uint8_t oy, void *pixels);

void gpu_display_buf(uint8_t xx, uint8_t yy, uint8_t ox, uint8_t oy, void *pixels);

#endif //MES_GPU_H
