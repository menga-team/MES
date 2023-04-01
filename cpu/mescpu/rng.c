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

#include "rng.h"
#include "libopencm3/stm32/adc.h"
#include "libopencm3/stm32/gpio.h"
#include "libopencm3/stm32/rcc.h"
#include "libopencm3/stm32/timer.h"
#include "libopencm3/stm32/adc.h"
#include "libopencm3/stm32/rcc.h"
#include "libopencm3/stm32/gpio.h"
#include "timer.h"
#include "tinymt32.h"
#include <stdint.h>

tinymt32_t tinymt;

void rng_init(void) {
    uint32_t seed = timer_get_ms();
    seed *= (TIM1_CNT+1);
    rcc_periph_clock_enable(RCC_ADC1);
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO7);
    adc_power_off(ADC1);
    adc_disable_scan_mode(ADC1);
    adc_disable_external_trigger_regular(ADC1);
    adc_set_right_aligned(ADC1);
    adc_enable_temperature_sensor();
    adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_1DOT5CYC);
    adc_power_on(ADC1);
    adc_start_conversion_direct(ADC1);
    while (!(adc_eoc(ADC1)))
        ;
    seed *= (adc_read_regular(ADC1)+1);
    tinymt32_init(&tinymt, seed);
}

uint32_t rng_u32(void) {
    return tinymt32_generate_uint32(&tinymt);
}
