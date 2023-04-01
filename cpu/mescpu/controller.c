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

#include "controller.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <malloc.h>
#include <memory.h>
#include <stdint.h>

uint8_t active_controller[4] = {0, 0, 0, 0};
uint8_t controller_timer[4] = {32, 32, 32, 32};
uint8_t controller_bits[4] = {0, 0, 0, 0};
uint8_t buttons[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int counter = 0;

void controller_setup_timers(void) {
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
    timer_set_oc_value(TIM1, TIM_OC2, 1); // OC2 serves as interrupt.
    nvic_enable_irq(NVIC_TIM1_CC_IRQ);
    nvic_set_priority(NVIC_TIM1_CC_IRQ, 0);
    timer_enable_irq(TIM1, TIM_DIER_CC2IE);
    timer_enable_counter(TIM1);
}

void tim1_cc_isr(void) {
        // gets executed at the rising edge.
        TIM_SR(TIM1) = 0x0000; // needed for timer

        uint8_t controller_bits[0] = gpio_get(CONTROLLER_PORT, CONTROLLER_PIN_0);
        uint8_t controller_bits[1] = gpio_get(CONTROLLER_PORT, CONTROLLER_PIN_1);
        uint8_t controller_bits[2] = gpio_get(CONTROLLER_PORT, CONTROLLER_PIN_2);
        uint8_t controller_bits[3] = gpio_get(CONTROLLER_PORT, CONTROLLER_PIN_3);
        for (int i = 0; i < 4; i++){
            if (controller_timer[i] >= 32){
                active_controller[i] = 0;
            }
            if (controller_timer[i] >= 8){
                if (controller_bits[i]){
                    controller_timer[i] = 0;
                    active_controller[i] = 1;
                }
            }
            if (controller_timer[i] < 8){
                buttons[(i*8)+controller_timer[i]] = controller_bits[i]
            }
            if (active_controller[i]){
                controller_timer[i]++;
            }

        }

//        uint8_t data_bit = gpio_get(DATA_PORT, DATA_PIN);
//        uint8_t controller_0_bit = gpio_get(SYNC_PORT, SYNC_PIN);
//        if (counter == 0) { // we wait for the syncbit to start counting
//                if (sync_bit) {
//                        counter++;
//                }
//        } else if (counter < 5) { // if we are registering the controller status, save sync_bit in he controller array
//                active_controller[counter - 1] = sync_bit;
//        } else { // if an unexpected sync_bit arrives reset the counter and start again
//                if (sync_bit) {
//                        counter = 1;
//                }
//        }
//        if (counter > 0) { // if we are counting, save the data_bit in the buttons array
//                buttons[counter - 1] = data_bit;
//                counter++;
//                if (counter >= 31) {
//                    counter = 0 //when we reach the last bit reset the counter
//                }
//        }
}

uint8_t controller_get_button_by_controller_and_index(int controller,
                                                      int button) {
    return buttons[(controller * 8) + button];
}

uint8_t *controller_get_buttons(int controller) {
    return buttons + controller * 8;
}

uint8_t controller_get_status(int controller) {
    return active_controller[controller];
}

uint8_t *controller_get_statuses(void) { return active_controller; }

void controller_configure_io(void) {
    gpio_set_mode(CLOCK_PORT, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, CLOCK_PIN);
}
