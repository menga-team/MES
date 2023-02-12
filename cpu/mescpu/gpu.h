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

/// Will blank the buffer using `black_with`.
void gpu_blank(Buffer buffer, uint8_t blank_with);

/// Will swap front and back buffer on the gpu.
void gpu_swap_buf(void);

/// Will send a buffer to the gpu.
void gpu_send_buf(Buffer buffer, uint8_t xx, uint8_t yy, uint8_t ox, uint8_t oy, void *pixels);

/// Will send a buffer to the back buffer of the gpu and then swap using 1 operation.
void gpu_display_buf(uint8_t xx, uint8_t yy, uint8_t ox, uint8_t oy, void *pixels);

/// Will block until next GPU READY is recieved, useful for frame timing.
void gpu_wait_for_next_ready(void);

#endif //MES_GPU_H
