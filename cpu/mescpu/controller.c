#include <stdint.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <memory.h>
#include <malloc.h>
#include "controller.h"

uint16_t active_controller[4] = {0, 0, 0, 0};
uint16_t buttons[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
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
        timer_set_prescaler(TIM1, (rcc_apb2_frequency / CONTROLLER_FREQ) - 1); // divides timer by value
        timer_set_period(TIM1, 100 + 1); // +1 for 50% duty cycle
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
        TIM_SR(TIM1) = 0x0000;
        uint16_t data_bit = gpio_get(DATA_PORT, DATA_PIN);
        uint16_t sync_bit = gpio_get(SYNC_PORT, SYNC_PIN);
        if (counter == 0) {
                if (sync_bit) {
                        counter++;
                }
        } else if (counter < 5) {
                active_controller[counter - 1] = sync_bit;
        } else {
                if (sync_bit) {
                        counter = 1;
                }
        }
        if (counter) {
                buttons[counter - 1] = data_bit;
                counter++;
                if (counter > 31) {

                }
        }
}

uint16_t controller_get_button_by_controller_and_index(int controller, int button) {
        return buttons[(controller * 8) + button];
}

uint16_t controller_get_button_copy_by_controller_and_index(int controller, int button) {
        uint16_t temp = buttons[(controller * 8) + button];
        return temp;
}

uint16_t controller_get_button_by_index(int button) {
        return buttons[button];
}

uint16_t controller_get_button_copy_by_index(int button) {
        uint16_t temp = buttons[button];
        return temp;
}

uint16_t **controller_get_buttons(int controller) {
        uint16_t **subset = malloc(sizeof(uint16_t *) * 8);
        for (int i = 0; i < 8; i++)
                subset[i] = &buttons[(controller * 4) + i];
        return subset;
}

uint16_t *controller_get_buttons_by_copy(int controller) {
        uint16_t *subset = malloc(sizeof(uint16_t) * 8);
        for (int i = 0; i < 8; i++)
                subset[i] = buttons[(controller * 4) + i];
        return subset;
}

uint16_t controller_get_status(int controller) {
        return active_controller[controller];
}

uint16_t controller_get_status_copy(int controller) {
        uint16_t temp = active_controller[controller];
        return temp;
}

uint16_t *controller_get_statuses(void) {
        return active_controller;
}

uint16_t *controller_get_statuses_copy(void) {
        uint16_t *subset = malloc(sizeof(uint16_t) * 4);
        memcpy(subset, active_controller, sizeof(uint16_t) * 4);
        return subset;
}

void controller_configure_io(void) {
        gpio_set_mode(CLOCK_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, CLOCK_PIN);
}