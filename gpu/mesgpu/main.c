#include "main.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <limits.h>
#include "gpu.c"

int main() {
        setup_clock();
        setup_gpio();
        while (1) {
                gpio_toggle(GPIOC, GPIO13);
                for (int i = 0; i < 80000; i++)
                        __asm__("nop");
        }
}

static void setup_clock() {
        rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE16_72MHZ]);
}

static void setup_gpio() {
        rcc_periph_clock_enable(RCC_GPIOA);
        rcc_periph_clock_enable(RCC_GPIOB);
        rcc_periph_clock_enable(RCC_GPIOC);
        gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13); // built-in
        gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO0); // hsync (gelb)
        gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO1); // vsync (orange)

        gpio_set(GPIOA, GPIO0);
        gpio_set(GPIOA, GPIO1);
}

static void setup_timers() {
        rcc_periph_clock_enable(RCC_TIM2);
        nvic_enable_irq(NVIC_TIM2_IRQ);
        rcc_periph_reset_pulse(RST_TIM2);
        timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT,
                       TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
        timer_set_prescaler(TIM2, 11439920); // 72MHz / 11.439920MHz = 6.293750MHz (pixel freq)
        timer_disable_preload(TIM2);
        timer_continuous_mode(TIM2);
        timer_set_period(TIM2, UINT16_MAX);
        timer_set_oc_value(TIM2, TIM_OC1, UINT16_MAX);
        timer_enable_counter(TIM2);
        timer_enable_irq(TIM2, TIM_DIER_CC1IE);
}

void tim2_isr(void)
{
        if (timer_get_flag(TIM2, TIM_SR_CC1IF)) {

                timer_clear_flag(TIM2, TIM_SR_CC1IF);
                uint32_t tick = timer_get_counter(TIM2);
                if(tick == UINT16_MAX)
                        overflow_counter++;
                if(overflow_counter > 0)
                        tick = overflow_counter << (sizeof(uint16_t) * CHAR_BIT) | tick;

                if(tick % BUFFER_WIDTH + H_FRONT_PORCH == 0)
                        gpio_clear(GPIOA, GPIO0);
                if(tick % BUFFER_WIDTH + H_FRONT_PORCH + H_SYNC_PULSE == 0)
                        gpio_set(GPIOA, GPIO0);

                if(tick == VGA_OUT_HEIGHT + V_FRONT_PORCH)
                        gpio_clear(GPIOA, GPIO0);

                if(tick == VGA_OUT_HEIGHT + V_FRONT_PORCH + V_SYNC_PULSE)
                        gpio_set(GPIOA, GPIO0);
        }
}