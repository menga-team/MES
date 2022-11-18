#include <stdint.h>
#include <limits.h>
#include <assert.h>
#include <libopencm3/stm32/gpio.h>

#ifndef MES_GPU_H
#define MES_GPU_H

// relevant timings:
// http://tinyvga.com/vga-timing/800x600@60Hz
#define H_DISPLAY_PIXELS 800
#define H_FRONT_PORCH_PIXELS 40
#define H_SYNC_PULSE_PIXELS 128
#define H_BACK_PORCH_PIXELS 88

#define V_DISPLAY_LINES 600
#define V_FRONT_PORCH_LINES 1
#define V_SYNC_PULSE_LINES 4
#define V_BACK_PORCH_LINES 23

#define H_WHOLE_LINE_PIXELS (H_DISPLAY_PIXELS + H_FRONT_PORCH_PIXELS + H_SYNC_PULSE_PIXELS + H_BACK_PORCH_PIXELS)
#define V_WHOLE_FRAME_LINES (V_DISPLAY_LINES + V_FRONT_PORCH_LINES + V_SYNC_PULSE_LINES + V_BACK_PORCH_LINES)
#define V_DISPLAY_LINES_PIXELS (V_DISPLAY_LINES * H_WHOLE_LINE_PIXELS)
#define V_FRONT_PORCH_LINES_PIXELS (V_FRONT_PORCH_LINES * H_WHOLE_LINE_PIXELS)
#define V_SYNC_PULSE_LINES_PIXELS (V_SYNC_PULSE_LINES * H_WHOLE_LINE_PIXELS)
#define V_BACK_PORCH_LINES_PIXELS (V_BACK_PORCH_LINES * H_WHOLE_LINE_PIXELS)
#define WHOLE_DISPLAY_PIXELS (V_WHOLE_FRAME_LINES*H_WHOLE_LINE_PIXELS)
#define PIXEL_CLOCK 40000000
// 0 -> idle high, active low
// 1 -> idle low, active high
#define H_SYNC_POLARITY 1
#define V_SYNC_POLARITY 1

// pixel buffer
#define VGA_OUT_REFRESH_RATE 60
#define BUFFER_TO_VIDEO_RATIO 5
#define BUFFER_WIDTH (H_DISPLAY_PIXELS / BUFFER_TO_VIDEO_RATIO)
#define BUFFER_HEIGHT (V_DISPLAY_LINES / BUFFER_TO_VIDEO_RATIO)
#define BUFFER_SIZE ((BUFFER_WIDTH * BUFFER_HEIGHT * BUFFER_BPP) / (CHAR_BIT * sizeof(uint8_t)))
#define BUFFER_BPP 3
#define DRM_BUFFER_BPP 6
#define PIXEL_MASK ((1 << BUFFER_BPP) - 1)

// colors
#define GPIO_COLOR_PORT GPIOB

/// Most Significant Pin
#define RED_MSP GPIO11
/// Middle Most Significant Pin
#define RED_MMSP GPIO10
/// Least Significant Pin
#define RED_LSP GPIO1

/// Most Significant Pin
#define GREEN_MSP GPIO12
/// Middle Most Significant Pin
#define GREEN_MMSP GPIO13
/// Least Significant Pin
#define GREEN_LSP GPIO14

/// Most Significant Pin
#define BLUE_MSP GPIO7
/// Middle Most Significant Pin
#define BLUE_MMSP GPIO8
/// Least Significant Pin
#define BLUE_LSP GPIO9

#define GPU_READY_PORT GPIOC
#define GPU_READY GPIO15

// -11 because we need some time to get ready
#define PREPARE_DISPLAY ((H_SYNC_PULSE_PIXELS / 5 + H_BACK_PORCH_PIXELS / 5) - 11) // 42-11 (43.2)

#define BUFFER_A_ADDRESS 0x20000000
#define BUFFER_B_ADDRESS 0x20001c20

#define OPERATION_LENGTH 8
#define OPERATION_DATA_LENGTH 512

enum Stage {
    READY = 0, UNHANDELED_OPERATION = 1, WAITING_FOR_DATA = 2, UNHANDELED_DATA = 3, WAITING_FOR_DMA = 4
} __attribute__ ((__packed__));

extern uint16_t color_palette[1 << DRM_BUFFER_BPP];
extern uint8_t buffer_a[(BUFFER_WIDTH * BUFFER_HEIGHT * BUFFER_BPP) / (CHAR_BIT * sizeof(uint8_t))];
extern uint8_t buffer_b[(BUFFER_WIDTH * BUFFER_HEIGHT * BUFFER_BPP) / (CHAR_BIT * sizeof(uint8_t))];
extern uint8_t *front_buffer, *back_buffer;
extern uint8_t buffer_line;
extern const void *line;
extern uint32_t pxs;
extern uint16_t data_cursor;
extern uint8_t operation[OPERATION_LENGTH];
extern uint8_t operation_data[OPERATION_DATA_LENGTH];
extern volatile enum Stage processing_stage;
extern const char *stage_pretty_names[5];
extern volatile bool run;

uint16_t get_port_config_for_color(uint8_t red, uint8_t green, uint8_t blue);

uint8_t gpu_get_pixel(const void *buffer, uint16_t position);

void gpu_set_pixel(void *buffer, uint16_t position, uint8_t data);

void gpu_swap_buffers(void);

void gpu_init(void);

void gpu_write(uint8_t x, uint8_t y, uint8_t fg, uint8_t bg, const char *text);

#endif