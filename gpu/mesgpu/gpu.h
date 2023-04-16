#include <libopencm3/stm32/gpio.h>
#include <limits.h>
#include <stdint.h>
#include "mesgraphics.h"

#ifndef MES_GPU_H
#define MES_GPU_H

// relevant timings:
// http://tinyvga.com/vga-timing/640x480@60Hz
#define H_DISPLAY_PIXELS 640
#define H_FRONT_PORCH_PIXELS 16
#define H_SYNC_PULSE_PIXELS 96
#define H_BACK_PORCH_PIXELS 48

#define V_DISPLAY_LINES 480
#define V_FRONT_PORCH_LINES 10
#define V_SYNC_PULSE_LINES 2
#define V_BACK_PORCH_LINES 33

#define H_WHOLE_LINE_PIXELS                                                    \
    (H_DISPLAY_PIXELS + H_FRONT_PORCH_PIXELS + H_SYNC_PULSE_PIXELS +           \
     H_BACK_PORCH_PIXELS)
#define V_WHOLE_FRAME_LINES                                                    \
    (V_DISPLAY_LINES + V_FRONT_PORCH_LINES + V_SYNC_PULSE_LINES +              \
     V_BACK_PORCH_LINES)
#define V_DISPLAY_LINES_PIXELS (V_DISPLAY_LINES * H_WHOLE_LINE_PIXELS)
#define V_FRONT_PORCH_LINES_PIXELS (V_FRONT_PORCH_LINES * H_WHOLE_LINE_PIXELS)
#define V_SYNC_PULSE_LINES_PIXELS (V_SYNC_PULSE_LINES * H_WHOLE_LINE_PIXELS)
#define V_BACK_PORCH_LINES_PIXELS (V_BACK_PORCH_LINES * H_WHOLE_LINE_PIXELS)
#define WHOLE_DISPLAY_PIXELS (V_WHOLE_FRAME_LINES * H_WHOLE_LINE_PIXELS)
#define PIXEL_CLOCK 25175000
// 0 -> idle high, active low
// 1 -> idle low, active high
#define H_SYNC_POLARITY 0
#define V_SYNC_POLARITY 0

// pixel buffer
#define VGA_OUT_REFRESH_RATE 60
#define BUFFER_TO_VIDEO_RATIO 4
#define BUFFER_WIDTH (H_DISPLAY_PIXELS / BUFFER_TO_VIDEO_RATIO)
#define BUFFER_HEIGHT (V_DISPLAY_LINES / BUFFER_TO_VIDEO_RATIO)
#define SCREEN_BUFFER_SIZE                                                     \
    ((BUFFER_WIDTH * BUFFER_HEIGHT * BUFFER_BPP) / CHAR_BIT)
#define BUFFER_BPP 3
#define DRM_BUFFER_BPP 6
#define PIXEL_MASK ((1 << BUFFER_BPP) - 1)

// colors
#define GPIO_COLOR_PORT GPIOB

/// Most Significant Pin
#define RED_MSP GPIO1
/// Middle Most Significant Pin
#define RED_MMSP GPIO10
/// Least Significant Pin
#define RED_LSP GPIO11

/// Most Significant Pin
#define GREEN_MSP GPIO12
/// Middle Most Significant Pin
#define GREEN_MMSP GPIO13
/// Least Significant Pin
#define GREEN_LSP GPIO14

/// Most Significant Pin
#define BLUE_MSP GPIO7
/// Middle Most Significant Pin
#define BLUE_MMSP GPIO6
/// Least Significant Pin
#define BLUE_LSP GPIO5

#define GPU_READY_PORT GPIOC
#define GPU_READY GPIO15

#define BUFFER_A_ADDRESS 0x20000000
#define BUFFER_B_ADDRESS 0x20001c20

#define OPERATION_LENGTH 8
#define OPERATION_DATA_LENGTH 4096

#define OPERATION_ID_INIT 0xff
#define OPERATION_ID_SEND_BUF 0x00
#define OPERATION_ID_PRINT_TEXT 0x01
#define OPERATION_ID_SWAP_BUF 0x02
#define OPERATION_ID_CHANGE_PALETTE 0x03
#define OPERATION_ID_DELAY_VSYNC 0x04
#define OPERATION_ID_DELAY_HSYNC 0x05
#define OPERATION_ID_WAIT_FRAME 0x06
#define OPERATION_ID_PRINT_TRANSPARENT 0x07
#define OPERATION_ID_DISPLAY_BUF 0x08
#define OPERATION_ID_PATCH_FONT 0x09
#define OPERATION_ID_RESET 0x0a
#define OPERATION_ID_BLANK 0x0b

#define INTERNAL_SHOW_STARTUP 0x01
#define INTERNAL_ADJUST_BRIGHTNESS 0x02
#define INTERNAL_DRAW_SDCARD 0x03

#define OPERATION_ID(x) x[3]
#define OPERATION_BUFFER(x) ((x[2] == 0x00) ? front_buffer : back_buffer)
#define INTERNAL_OPERATION(x) (x[0] == 0xff)

enum Stage {
    UNINITIALIZED = 0,
    READY = 1,
    UNHANDELED_OPERATION = 2,
    WAITING_FOR_DATA = 3,
    UNHANDELED_DATA = 4,
    WAITING_FOR_DMA = 5
} __attribute__((__packed__));

extern uint16_t color_palette[1 << BUFFER_BPP];
extern uint8_t buffer_a[(BUFFER_WIDTH * BUFFER_HEIGHT * BUFFER_BPP) /
                        (CHAR_BIT * sizeof(uint8_t))];
extern uint8_t buffer_b[(BUFFER_WIDTH * BUFFER_HEIGHT * BUFFER_BPP) /
                        (CHAR_BIT * sizeof(uint8_t))];
extern uint8_t *front_buffer, *back_buffer;
extern uint8_t buffer_line;
extern int8_t sub_line;
extern const void *line;
extern uint32_t pxs;
extern uint8_t operation[OPERATION_LENGTH];
extern uint8_t operation_data[OPERATION_DATA_LENGTH];
extern volatile enum Stage processing_stage;
extern volatile uint16_t gpu_ready_port;
extern const char *stage_pretty_names[6];
extern volatile bool run;

void gpu_reset_palette(void);

uint8_t gpu_get_pixel(const void *buffer, uint16_t position);

void gpu_set_pixel(void *buffer, uint16_t position, uint8_t data);

void gpu_swap_buffers(void);

void gpu_blank(void *buffer, uint8_t blank_with);

void gpu_init(void);

void gpu_write(void *buffer, uint8_t x, uint8_t y, uint8_t fg, uint8_t bg,
               const char *text);

void gpu_insert_buf(void *buffer, const uint8_t *image, const uint8_t widgh,
                    const uint8_t height, const uint8_t x, const uint8_t y);

void gpu_patch_font(void *patch, uint8_t start, uint8_t end);

void gpu_reset(void);

#endif
