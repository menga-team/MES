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

#define BLUE_PIN_1 GPIO7 // most significant bit
#define BLUE_PIN_2 GPIO8 // least significant bit

static void setup_clock(void);
static void setup_output(void);
static void start_video(void);
static void set_color(uint8_t color);
static void reset_color(void);

int main(void);

#endif