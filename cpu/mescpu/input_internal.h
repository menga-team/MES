/* This file is part of MES.
 *
 * MES is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MES is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warrantyp of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MES. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef MES_INPUT_INTERNAL
#define MES_INPUT_INTERNAL

#include "input.h"
#include <libopencm3/stm32/gpio.h>
#include <stdint.h>

#define CONTROLLER_PIN_0 GPIO9
#define CONTROLLER_PIN_1 GPIO10
#define CONTROLLER_PIN_2 GPIO11
#define CONTROLLER_PIN_3 GPIO12
#define CONTROLLER_PINS                                                        \
    (CONTROLLER_PIN_0 | CONTROLLER_PIN_1 | CONTROLLER_PIN_2 | CONTROLLER_PIN_3)
#define CLOCK_PIN GPIO_TIM1_CH1 // PA8
#define DATA_PIN GPIO11

#define CLOCK_PORT GPIOA
#define CONTROLLER_PORT GPIOA

static const uint16_t CONTROLLER_PIN_MAP[] = {
    [CONTROLLER_1] = CONTROLLER_PIN_0,
    [CONTROLLER_2] = CONTROLLER_PIN_1,
    [CONTROLLER_3] = CONTROLLER_PIN_2,
    [CONTROLLER_4] = CONTROLLER_PIN_3,
};

extern volatile uint8_t controller_counters[4];

void input_setup(void);

#endif // MES_INPUT_INTERNAL
