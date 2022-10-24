#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include "mesmath.h"

static void setup_clock(void) {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
}

static void setup_pins(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOD);

    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO12);
//    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);

}

int freq = 500;

int main(void) {
    setup_pins();
    setup_clock();
    rcc_periph_clock_enable(RCC_TIM3);
    rcc_periph_reset_pulse(RST_TIM3); //resets settings
    timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_prescaler(TIM3, (72000000/freq) - 1); // divides timer by value
    timer_set_period(TIM3, 100); // counts untill inclusive 100
    timer_disable_preload(TIM3);
    timer_continuous_mode(TIM3);
    timer_set_oc_value(TIM3, TIM_OC1, 10); // sets intervl for code execution
    timer_set_counter(TIM3, 0);
    timer_enable_oc_preload(TIM3, TIM_OC1);
    nvic_enable_irq(NVIC_TIM3_IRQ);
    nvic_set_priority(NVIC_TIM3_IRQ, 0);
    timer_enable_irq(TIM3, TIM_DIER_CC1IE);


    timer_enable_counter(TIM3);
}

int same_bit_counter = 0;
int same_bit_error_threshhold = 100;
int wait_for_next_sync = 1;
int last_bit = 0;
int bit_index = 0;
int current_button = 0;
int raw_bits[] = {
        0, 0, 0,
        0, 0, 0,
        0, 0, 0
};
int compressed_bits[] = {
        0, 0, 0,                // button 1
        0, 0, 0,                // button 2
        0, 0, 0,                // button 3
        0, 0, 0,                // button 4
        0, 0, 0,                // button 5
        0, 0, 0,                // button 6
        0, 0, 0,                // button 7
        0, 0, 0,                // button 8
        0, 0, 0,                // sync sequence
};
int buttons[] = {0, 0, 0, 0, 0, 0, 0, 0};
//int[] prev_buttons = {0, 0, 0, 0, 0, 0, 0, 0};

void process_bits() {
    int i;
    int local_buttons[] = {0, 0, 0, 0, 0, 0, 0, 0};
    for (i = 0; i < 9; i = i + 3) {
        if (compressed_bits[i] == compressed_bits[i + 2] && compressed_bits[i] != compressed_bits[i + 1]) {
            return; // check for 010 or 101 combinations which are errors
        }
        if (compressed_bits[i] == 0) {
            return; // check for 0xx combinations which are errors
        }
        if (i < 8 && compressed_bits[i] == compressed_bits[i + 1] == compressed_bits[i + 2] == 1) {
            return; // check for 111 combinations which are errors
        }
        local_buttons[i / 3] = compressed_bits[1 + 1];
    }
    for (i = 0; i < 8; i++) {
        buttons[i] = local_buttons[i];
    }

}

void read_no_clock_bits() {
    uint16_t little_bit = gpio_get(GPIOB, GPIO13);
    raw_bits[bit_index] = little_bit;
    if (bit_index == 2) {
        int sum = raw_bits[0] + raw_bits[1] + raw_bits[2];
        if (sum > 1) {
            compressed_bits[current_button * 3] = 1;
        } else {
            bit_index = -1;
        }
    }
    if (bit_index == 5) {
        int sum = raw_bits[3] + raw_bits[4] + raw_bits[5];
        if (sum > 1) {
            compressed_bits[current_button * 3 + 1] = 1;
        } else {
            compressed_bits[current_button * 3 + 1] = 0;
            compressed_bits[current_button * 3 + 2] = 0;
            bit_index = -1;
            current_button += 1;
        }
    }
    if (bit_index == 8) {
        int sum = raw_bits[6] + raw_bits[7] + raw_bits[8];
        if (sum > 1) {
            if (current_button == 9) {
                process_bits();
            }
            current_button = 0;
            bit_index = -1;
        } else {
            compressed_bits[current_button * 3 + 1] = 0;
            bit_index = -1;
            current_button += 1;
        }
    }
    bit_index += 1;
}

void tim3_isr(void) {
    uint32_t count = TIM_CNT(TIM3);
//    if (count % 2 == 0) {
//        gpio_toggle(GPIOB, GPIO12);
//    }
    read_no_clock_bits();
}