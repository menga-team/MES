#ifndef MES_MESGRAPHICS_H
#define MES_MESGRAPHICS_H

#include <math.h>
#include "stdint.h"
#include "stdlib.h"
#include <stdbool.h>

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
#define BUFFER_SIZE(X, Y) (((uint16_t)ceil((float)(BUFFER_BPP * ((X) * (Y))) / 8.0) % BUFFER_BPP) * 3 + ((BUFFER_BPP * ((X) * (Y))) / 8))
#define BUFFER_POSITION(RECT, X, Y) ((Y) * (RECT)->width + (X))
#define PIXEL_MASK ((1 << BUFFER_BPP) - 1)

#define vector3_subtract(L, R) (L)->x -= (R)->x; (L)->y -= (R)->y; (L)->z -= (R)->z
#define vector3_add(L, R) (L)->x += (R)->x; (L)->y += (R)->y; (L)->z += (R)->z
#define vector3_divide(L, R) (L)->x /= (R); (L)->y /= (R); (L)->z /= (R)

struct RectangularBuffer {
    uint8_t width;
    uint8_t height;
    void *data;
};

struct Vector3f {
    float x;
    float y;
    float z;
};

struct Vector3 {
    uint8_t x;
    uint8_t y;
    uint8_t z;
};

struct Vector2 {
    uint8_t x;
    uint8_t y;
};

typedef struct RectangularBuffer RectangularBuffer;
typedef struct Vector3f Vector3f;
typedef struct Vector3 Vector3;
typedef struct Vector2 Vector2;

static void swap(uint8_t *xp, uint8_t *yp) {
        uint8_t temp = *xp;
        *xp = *yp;
        *yp = temp;
}

static Vector2 vector2_create(uint8_t x, uint8_t y) {
        return (Vector2) {x, y};
}

static Vector3f vector3f_create(float x, float y, float z) {
        return (Vector3f) {x, y, z};
}

static Vector3 vector3_create(uint8_t x, uint8_t y, uint8_t z) {
        return (Vector3) {x, y, z};
}

static Vector3f vector3_to_float(Vector3 *vec) {
        return (Vector3f) {vec->x, vec->y, vec->z};
}

static Vector3 vector3f_round(Vector3f *vec) {
        return (Vector3) {lroundf(vec->x), lroundf(vec->y), lroundf(vec->z)};
}

static RectangularBuffer buffer_create(uint8_t width, uint8_t height) {
        return (RectangularBuffer) {
                width, height, malloc(BUFFER_SIZE(width, height))
        };
}

static void buffer_resize(RectangularBuffer *rect, uint8_t width, uint8_t height) {
        rect->width = width;
        rect->height = height;
        rect->data = realloc(rect->data, BUFFER_SIZE(width, height));
}

static void buffer_destory(RectangularBuffer *rect) {
        free(rect->data);
        free(rect);
}

static void buffer_set_pixel(RectangularBuffer *rect, uint8_t x, uint8_t y, uint8_t color) {
        uint16_t pos = BUFFER_POSITION(rect, x, y);
        uint32_t *pixels = (uint32_t *) (rect->data + (pos / 8) * BUFFER_BPP);
        uint8_t shift = (7 - (pos % 8)) * BUFFER_BPP;
        uint32_t mask = PIXEL_MASK << shift;
        *pixels = (*pixels & ~mask) | ((color & PIXEL_MASK) << shift);
}

static uint8_t buffer_get_pixel(RectangularBuffer *rect, uint8_t x, uint8_t y) {
        uint16_t pos = BUFFER_POSITION(rect, x, y);
        uint32_t pixels = (*(uint32_t *) (rect->data + (pos / 8) * BUFFER_BPP));
        return (pixels >> ((7 - (pos % 8)) * BUFFER_BPP)) & PIXEL_MASK;
}

static void buffer_draw_line(RectangularBuffer *rect, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color) {
        bool steep = false;
        if (abs(x0 - x1) < abs(y0 - y1)) {
                swap(&x0, &y0);
                swap(&x1, &y1);
                steep = true;
        }
        if (x0 > x1) {
                swap(&x0, &x1);
                swap(&y0, &y1);
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

#endif
