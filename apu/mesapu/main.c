#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>

#define FREQUENCY 440

// precompiled sine wave
uint8_t wave[] = {127, 134, 142, 150, 158, 166, 173, 181, 188, 195, 201, 207, 213, 219, 224, 229, 234, 238, 241, 245, 247, 250, 251, 252, 253, 254, 253, 252, 251, 250, 247, 245, 241, 238, 234, 229, 224, 219, 213, 207, 201, 195, 188, 181, 173, 166, 158, 150, 142, 134, 127, 119, 111, 103, 95, 87, 80, 72, 65, 58, 52, 46, 40, 34, 29, 24, 19, 15, 12, 8, 6, 3, 2, 1, 0, 0, 0, 1, 2, 3, 6, 8, 12, 15, 19, 24, 29, 34, 40, 46, 52, 58, 65, 72, 80, 87, 95, 103, 111, 119,};
uint8_t c = 0;

static void setup_gpio(void) {
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, 0xFFFF);
}

static void setup_clock(void) {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
}

static void setup_timer(void) {
    rcc_periph_clock_enable(RCC_TIM2);
    rcc_periph_reset_pulse(RST_TIM2);
    timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_prescaler(TIM2, (72000000/(2*sizeof(wave)*FREQUENCY)) - 1);
    timer_set_period(TIM2, 1);
    timer_disable_preload(TIM2);
    timer_continuous_mode(TIM2);
    timer_set_counter(TIM2, 0);
    nvic_enable_irq(NVIC_TIM2_IRQ);
    nvic_set_priority(NVIC_TIM2_IRQ, 0);
    timer_enable_irq(TIM2, TIM_DIER_UIE);
    timer_enable_counter(TIM2);
}

int main(void) {
    setup_clock();
	setup_gpio();
    setup_timer();
    GPIO_ODR(GPIOA) = 0xffff;
    while (1) {

    }
	return 0;
}

void tim2_isr(void) {
    timer_clear_flag(TIM2, TIM_DIER_UIE);
    TIM_SR(TIM2) = 0x0000;
    GPIO_ODR(GPIOA) = wave[c++];
    if(c >= 100) c = 0;
}
