#include <stdint.h>
#include <libopencm3/stm32/gpio.h>


#ifndef MES_MAIN_H
#define MES_MAIN_H

#define GPIO_COLOR_PORT GPIOB

#define RED_PIN_1 GPIO11 // most significant bit
#define RED_PIN_2 GPIO10
#define RED_PIN_3 GPIO1 // least significant bit

#define GREEN_PIN_1 GPIO12 // most significant bit
#define GREEN_PIN_2 GPIO13
#define GREEN_PIN_3 GPIO14 // least significant bit

#define BLUE_PIN_1 GPIO8 // most significant bit
#define BLUE_PIN_2 GPIO9 // least significant bit

// (-4 is trial and error)
#define RESET_COLOR ((H_SYNC_PULSE_PIXELS / 5 + H_BACK_PORCH_PIXELS / 5 + H_DISPLAY_PIXELS / 5) - 4) // 202-4 (203.2)
#define START_DRAWING ((H_SYNC_PULSE_PIXELS / 5 + H_BACK_PORCH_PIXELS / 5) - 5) // 42-4 (43.2)

static void setup_clock(void);

static void setup_output(void);

static void start_video(void);

static void set_color(uint8_t color);

static void reset_color(void);

int main(void);

#endif