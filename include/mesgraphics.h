#ifndef MES_MESGRAPHICS_H
#define MES_MESGRAPHICS_H

#include "utils.h"
#include <stdint.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

/**
 * @param red: 0b000 - 0b111
 * @param green: 0b000 - 0b111
 * @param blue: 0b000 - 0b111
 * Generates a port config given the 3 colors.
 */
#define COLOR(red, green, blue)                                                \
    (uint16_t)(((red & 0b100) >> 1) | ((red & 0b010) << 9) |                   \
               ((red & 0b001) << 11) | ((green & 0b100) << 10) |               \
               ((green & 0b010) << 12) | ((green & 0b001) << 14) |             \
               ((blue & 0b100) << 5) | ((blue & 0b010) << 5) |                 \
               ((blue & 0b001) << 5))

#define BUFFER_BPP 3
#define BUFFER_SIZE(X, Y)                                                      \
    (((uint16_t)ceil((float)(BUFFER_BPP * ((X) * (Y))) / 8.0) % BUFFER_BPP) *  \
         3 +                                                                   \
     ((BUFFER_BPP * ((X) * (Y))) / 8))
#define BUFFER_POSITION(RECT, X, Y) ((Y) * (RECT)->width + (X))
#define PIXEL_MASK ((1 << BUFFER_BPP) - 1)

#define SWAP(A, B)                                                             \
    {                                                                          \
        __auto_type _swap_temp = *(A);                                        \
        *(A) = *(B);                                                           \
        *(B) = _swap_temp;                                                     \
    }

typedef struct {
    uint8_t width;
    uint8_t height;
    void *data;
} RectangularBuffer;

static RectangularBuffer UNUSED buffer_create(uint8_t width, uint8_t height) {
    return (RectangularBuffer){width, height,
                               malloc(BUFFER_SIZE(width, height))};
}

static void UNUSED buffer_resize(RectangularBuffer *rect, uint8_t width,
                          uint8_t height) {
    rect->width = width;
    rect->height = height;
    rect->data = realloc(rect->data, BUFFER_SIZE(width, height));
}

static void UNUSED buffer_destory(RectangularBuffer *rect) {
    free(rect->data);
    free(rect);
}

static void UNUSED buffer_set_pixel(RectangularBuffer *rect, uint8_t x, uint8_t y,
                             uint8_t color) {
    uint16_t pos = BUFFER_POSITION(rect, x, y);
    uint32_t *pixels = (uint32_t *)(rect->data + (pos / 8) * BUFFER_BPP);
    uint8_t shift = (7 - (pos % 8)) * BUFFER_BPP;
    uint32_t mask = PIXEL_MASK << shift;
    *pixels = (*pixels & ~mask) | ((color & PIXEL_MASK) << shift);
}

static uint8_t UNUSED buffer_get_pixel(RectangularBuffer *rect, uint8_t x, uint8_t y) {
    uint16_t pos = BUFFER_POSITION(rect, x, y);
    uint32_t pixels = (*(uint32_t *)(rect->data + (pos / 8) * BUFFER_BPP));
    return (pixels >> ((7 - (pos % 8)) * BUFFER_BPP)) & PIXEL_MASK;
}

static void UNUSED buffer_draw_line(RectangularBuffer *rect, uint8_t x0, uint8_t y0,
                             uint8_t x1, uint8_t y1, uint8_t color) {
    bool steep = false;
    if (abs(x0 - x1) < abs(y0 - y1)) {
        SWAP(&x0, &y0);
        SWAP(&x1, &y1);
        steep = true;
    }
    if (x0 > x1) {
        SWAP(&x0, &x1);
        SWAP(&y0, &y1);
    }
    int16_t dx = x1 - x0;
    int16_t dy = y1 - y0;
    int16_t derror2 = abs(dy) * 2;
    int16_t error2 = 0;
    uint8_t y = y0;
    for (uint8_t x = x0; x < x1; ++x) {
        if (steep) {
            buffer_set_pixel(rect, y, x, color);
        } else {
            buffer_set_pixel(rect, x, y, color);
        }
        error2 += derror2;
        if (error2 > dx) {
            y += (y1 > y0 ? 1 : -1);
            error2 -= dx * 2;
        }
    }
}

#endif // MES_MESGRAPHICS_H
