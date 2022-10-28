#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <string.h>
#include "mesmath.h"

#define sync_pin GPIO13
#define clock_pin GPIO12
#define data_pin GPIO11

#define sync_port GPIOB
#define clock_port GPIOB
#define data_port GPIOB

static void setup_clock(void) {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
}

static void setup_pins(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOD);
    rcc_periph_clock_enable(RCC_GPIOC);

    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO12);
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_TIM1_CH1N);
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
//    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);

}

int freq = 4000;

int main(void) {
    setup_pins();
    setup_clock();
    rcc_periph_clock_enable(RCC_TIM1);
    rcc_periph_reset_pulse(RST_TIM1); //resets settings
    timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_enable_break_main_output(TIM1);
    timer_set_prescaler(TIM1, (72000000/freq) - 1); // divides timer by value
    timer_set_period(TIM1, 100); // counts untill inclusive 100
    timer_disable_preload(TIM1);
    timer_continuous_mode(TIM1);
    timer_set_oc_value(TIM1, TIM_OC1N, 1); // sets intervl for code execution
    timer_set_oc_mode(TIM1, TIM_OC1N, TIM_OCM_PWM1);
    timer_set_oc_polarity_high(TIM_OC1N, TIM_OC1N);
    timer_enable_oc_preload(TIM1, TIM_OC1N);
    timer_enable_oc_output(TIM1, TIM_OC1N);
    timer_set_counter(TIM1, 0);
    timer_enable_oc_preload(TIM1, TIM_OC1N);
    nvic_enable_irq(NVIC_TIM1_CC_IRQ);
    nvic_set_priority(NVIC_TIM1_CC_IRQ, 0);
    timer_enable_irq(TIM1, TIM_DIER_CC1IE);


    timer_enable_counter(TIM1);
    double a = 1000000;

//    while (1) {
////        gpio_toggle(GPIOC, GPIO13);
//        for (int i = 0; i < a; i++) {
//            __asm__("nop");
//        }
//
//    } /*LED on/off */
}


uint16_t active_controller[4] = {0, 0, 0, 0};
uint16_t buttons[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int counter = 0;
void tim1_cc_isr(void) {
    uint16_t data_bit = gpio_get(data_port, data_pin);
    uint16_t sync_bit = gpio_get(sync_port, sync_pin);
    if (counter == 0) {
        if (sync_bit) {
            counter++;
        }
    }
    else if (counter < 5) {
        active_controller[counter-1] = sync_bit;
    }
    else {
        if (sync_bit) {
            counter = 1;
        }
    }
    if (counter) {
        buttons[counter-1] = data_bit;
        counter++;
    if (counter > 31) {

    }
    }
}
/*
 * gets button state by controller index and button index
 *
 * @param controller: index of controller
 * @param button: index of button
 *
 * @return: button state
 */
static uint16_t get_button_by_contorller_and_index(int controller, int button) {
    return buttons[(controller*4) + button];
}

/*
 * return a copy of the button state by controller index and button index
 *
 * @param controller: index of controller
 * @param button: index of button
 *
 * @return: copy of button state
 */
static uint16_t get_button_by_contorller_and_index_copy(int controller, int button) {
    uint16_t temp = buttons[(controller*4) + button];
    return temp;
}

/*
 * gets button state by button index
 *
 * @param button: index of button
 *
 * @return: button state
 */
static uint16_t get_button_by_index(int button) {
    return buttons[button];
}

/*
 * return a copy of the button state by button index
 *
 * @param button: index of button
 *
 * @return: copy of button state
 */
static uint16_t get_button_by_index_copy(int button) {
    uint16_t temp = buttons[button];
    return temp;
}

/*
 * gets array of button states by controller index
 *
 * @param controller: index of controller
 *
 * @return: array button states
 */
static uint16_t * get_buttons_by_controller(int controller) {
    uint16_t * subset[8] = {0, 0, 0, 0, 0, 0, 0, 0 };
    for(int i=0; i<8; i++)
        subset[i] = &buttons[(controller*4) + i];
    return subset;
}

/*
 * gets copy of array of button states by controller index
 *
 * @param controller: index of controller
 *
 * @return: copy of array button states
 */
static uint16_t * get_buttons_by_controller_copy(int controller) {
    uint16_t subset[8] = {0, 0, 0, 0, 0, 0, 0, 0 };
    for(int i=0; i<8; i++)
        subset[i] = buttons[(controller*4) + i];
    return subset;
}

/*
 * gets controller status
 *
 * @param controller: index of controller
 *
 * @return: status of controller
 */
static uint16_t get_controller_status(int controller) {
    return active_controller[controller];
}

/*
 * gets copy of controller status
 *
 * @param controller: index of controller
 *
 * @return: copy of status of controller
 */
static uint16_t get_controller_status_copy(int controller) {
    uint16_t temp = active_controller[controller];
    return temp;
}

/*
 * gets array of controller statuses
 *
 * @return: array of controller status
 */
static uint16_t * get_controller_statuses() {
    return active_controller;
}

/*
 * gets copy of array of controller statuses
 *
 * @return: copy of array of controller status
 */
static uint16_t * get_controller_statuses_copy() {
    uint16_t subset[4];
    memcpy(subset, active_controller, sizeof(uint16_t)*4);
    return subset;
}


//int same_bit_counter = 0;
//int same_bit_error_threshhold = 100;
//int wait_for_next_sync = 1;
//int last_bit = 0;
//int bit_index = 0;
//int current_button = 0;
//int raw_bits[] = {
//        0, 0, 0,
//        0, 0, 0,
//        0, 0, 0
//};
//int compressed_bits[] = {
//        0, 0, 0,                // button 1
//        0, 0, 0,                // button 2
//        0, 0, 0,                // button 3
//        0, 0, 0,                // button 4
//        0, 0, 0,                // button 5
//        0, 0, 0,                // button 6
//        0, 0, 0,                // button 7
//        0, 0, 0,                // button 8
//        0, 0, 0,                // sync sequence
//};
//int buttons[] = {0, 0, 0, 0, 0, 0, 0, 0};
////int[] prev_buttons = {0, 0, 0, 0, 0, 0, 0, 0};
//
//static void process_bits() {
//    int i;
//    int local_buttons[] = {0, 0, 0, 0, 0, 0, 0, 0};
//    for (i = 0; i < 9; i = i + 3) {
//        if (compressed_bits[i] == compressed_bits[i + 2] && compressed_bits[i] != compressed_bits[i + 1]) {
//            return; // check for 010 or 101 combinations which are errors
//        }
//        if (compressed_bits[i] == 0) {
//            return; // check for 0xx combinations which are errors
//        }
//        if (i < 8 && (compressed_bits[i] == compressed_bits[i + 1]) == compressed_bits[i + 2] == 1) {
//            return; // check for 111 combinations which are errors
//        }
//        local_buttons[i / 3] = compressed_bits[1 + 1];
//    }
//    for (i = 0; i < 8; i++) {
//        buttons[i] = local_buttons[i];
//    }
//
//}
//
//static void read_no_clock_bits() {
//    uint16_t little_bit = gpio_get(GPIOB, GPIO13);
//    raw_bits[bit_index] = little_bit;
//    if (bit_index == 2) {
//        int sum = raw_bits[0] + raw_bits[1] + raw_bits[2];
//        if (sum > 1) {
//            compressed_bits[current_button * 3] = 1;
//        } else {
//            bit_index = -1;
//        }
//    }
//    if (bit_index == 5) {
//        int sum = raw_bits[3] + raw_bits[4] + raw_bits[5];
//        if (sum > 1) {
//            compressed_bits[current_button * 3 + 1] = 1;
//        } else {
//            compressed_bits[current_button * 3 + 1] = 0;
//            compressed_bits[current_button * 3 + 2] = 0;
//            bit_index = -1;
//            current_button += 1;
//        }
//    }
//    if (bit_index == 8) {
//        int sum = raw_bits[6] + raw_bits[7] + raw_bits[8];
//        if (sum > 1) {
//            if (current_button == 9) {
//                process_bits();
//            }
//            current_button = 0;
//            bit_index = -1;
//        } else {
//            compressed_bits[current_button * 3 + 1] = 0;
//            bit_index = -1;
//            current_button += 1;
//        }
//    }
//    bit_index += 1;
//}

//void tim3_isr(void) {
//    uint32_t count = TIM_CNT(TIM3);
//    if (count % 3 == 0) {
//        gpio_toggle(GPIOB, GPIO12);
//    }
//    if (count % 2 == 0) {
//        read_no_clock_bits();
//    }
////        gpio_toggle(GPIOC, GPIO13);
//}