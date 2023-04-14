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

#ifndef MES_CONTROLLER_INTERNAL_H
#define MES_CONTROLLER_INTERNAL_H

#define CONTROLLER_PIN_0 GPIO9
#define CONTROLLER_PIN_1 GPIO10
#define CONTROLLER_PIN_2 GPIO11
#define CONTROLLER_PIN_3 GPIO12
#define CLOCK_PIN GPIO_TIM1_CH1 // PA8
#define DATA_PIN GPIO11

#define SYNC_PORT GPIOB
#define CLOCK_PORT GPIOA
#define CONTROLLER_PORT GPIOA
#define DATA_PORT GPIOB

#define CONTROLLER_FREQ 4000

extern uint16_t active_controller[4];
extern uint16_t buttons[32];
extern int counter;

/**
 * Configures PINs for communicating with controllers
 */
void controller_configure_io(void);

/**
 * Initialize timers needed for controllers.
 */
void controller_setup_timers(void);

#endif // MES_CONTROLLER_INTERNAL_H
