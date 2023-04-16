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

#include "input.h"
#include "input_internal.h"
#include "libopencm3/stm32/f1/gpio.h"
#include "libopencm3/stm32/gpio.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <stdint.h>

#define CONTROLLER_FREQ (1000 * 100)

volatile bool controller_available[4] = {false};
volatile bool controller_buttons[4][8] = {{0}};

/* 0  -> awaiting sync bit
 * 1  -> sync bit
 * 2  -> first button
 * ...
 * 9  -> last button
 * 16 -> end of transmission, will reset to 0
 */
volatile uint8_t controller_counters[4] = {0};

void tim1_cc_isr(void) {
    // gets executed at the falling edge.
    TIM_SR(TIM1) = 0x0000;
    uint16_t port = gpio_port_read(CONTROLLER_PORT);

    if (!controller_available[CONTROLLER_1] && (port & CONTROLLER_PIN_0)) {
        controller_available[CONTROLLER_1] = true;
    }
    if (!controller_available[CONTROLLER_2] && (port & CONTROLLER_PIN_1)) {
        controller_available[CONTROLLER_2] = true;
    }
    if (!controller_available[CONTROLLER_3] && (port & CONTROLLER_PIN_2)) {
        controller_available[CONTROLLER_3] = true;
    }
    if (!controller_available[CONTROLLER_4] && (port & CONTROLLER_PIN_3)) {
        controller_available[CONTROLLER_4] = true;
    }

    for (uint8_t i = 0; i < 4; ++i) {
        if (!controller_available[i]) {
            continue;
        }
        controller_counters[i]++;
        // reached end of transmission.
        if (controller_counters[i] == 16) {
            controller_counters[i] = 0;
            continue;
        }
        // the controller only has 8 buttons.
        if (controller_counters[i] > 9) {
            continue;
        }
        // check if controller is still here.
        if (controller_counters[i] == 1) {
            // no sync bit, controller is no longer available.
            if (!(port & CONTROLLER_PIN_MAP[i])) {
                controller_available[i] = false;
                controller_counters[i] = 0;
            }
            // controller is here, wait for buttons to be sent.
            continue;
        }
        // register pressed/released button.
        uint8_t button = controller_counters[i] - 2;
        controller_buttons[i][button] = port & CONTROLLER_PIN_MAP[i];
    }
}

void input_setup(void) {
    gpio_set_mode(CLOCK_PORT, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, CLOCK_PIN);
    gpio_set_mode(CONTROLLER_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                  CONTROLLER_PINS);
    // pull down
    gpio_clear(CONTROLLER_PORT, CONTROLLER_PINS);
    
    /*
     * TIM1 generates a clock with 50% duty cycle on PIN A8.
     * 0-50   -> high
     * 51-101 -> low
     */
    rcc_periph_clock_enable(RCC_TIM1);
    rcc_periph_reset_pulse(RST_TIM1);
    timer_enable_break_main_output(TIM1);
    timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_prescaler(TIM1, (rcc_apb2_frequency / CONTROLLER_FREQ) -
                                  1); // divides timer by value
    timer_set_period(TIM1, 100 + 1);  // +1 for 50% duty cycle
    timer_disable_preload(TIM1);
    timer_continuous_mode(TIM1);
    timer_set_counter(TIM1, 0);
    timer_enable_oc_preload(TIM1, TIM_OC1);
    timer_enable_oc_output(TIM1, TIM_OC1);
    timer_set_oc_mode(TIM1, TIM_OC1, TIM_OCM_PWM1);
    timer_set_oc_value(TIM1, TIM_OC1, 50 + 1);
    timer_set_oc_polarity_high(TIM1, TIM_OC1);
    timer_set_counter(TIM1, 0);
    timer_set_oc_value(TIM1, TIM_OC2, 50 + 1); // OC2 serves as interrupt.
    nvic_enable_irq(NVIC_TIM1_CC_IRQ);
    nvic_set_priority(NVIC_TIM1_CC_IRQ, 0);
    timer_enable_irq(TIM1, TIM_DIER_CC2IE);
    timer_enable_counter(TIM1);
}

void input_change_freq(uint16_t freq) {
    timer_set_prescaler(TIM1, (rcc_apb2_frequency / (freq * 100)) - 1);
}

bool input_get_button(uint8_t controller, uint8_t button) {
    return controller_buttons[controller][button];
}

volatile bool *input_get_buttons(uint8_t controller) {
    return controller_buttons[controller];
}

bool input_is_available(uint8_t controller) {
    return controller_available[controller];
}

volatile bool *input_get_availability(void) { return *&controller_available; }
