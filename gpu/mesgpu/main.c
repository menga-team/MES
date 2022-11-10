#include "main.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/dma.h>
#include <mesmath.h>
#include <stdio.h>
#include "gpu.h"

// Include peppers image as `peppers`
//#include "images/peppers.m3ifc"
// Include error screen as 'error'
#include "images/error.m3ifc"

// the bits per pixel (bpp) define how large the palette can be.
// colorid => port

int main(void) {
        rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
        setup_output();
        gpu_init();
        setup_video();
        start_communication();
        start_video();
        char *operation_string = malloc(sizeof(char) * 24 + 1); // 24chars + NUL
        while (1) {
                // we have ~5µs every line and ~770µs every frame
                sprintf(
                        operation_string,
                        "%02x %02x %02x %02x  %02x %02x %02x %02x",
                        operation[0],
                        operation[1],
                        operation[2],
                        operation[3],
                        operation[4],
                        operation[5],
                        operation[6],
                        operation[7]);
                write(8, 96, 1, 0, operation_string);
        }
}

void setup_output(void) {
        rcc_periph_clock_enable(RCC_AFIO);
        rcc_periph_clock_enable(RCC_GPIOA);
        rcc_periph_clock_enable(RCC_GPIOB);
        rcc_periph_clock_enable(RCC_GPIOC);
        gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13); // built-in
        gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPU_READY_PIN);
        gpio_set(GPIOC, GPIO13);
        // A8: hsync (yellow cable)
        // B0: vsync (orange cable)
        gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_TIM1_CH1); // PA8
        gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_TIM3_CH3); // PB0
        gpio_set_mode(GPIO_COLOR_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                      RED_MSP | RED_MMSP | RED_LSP |
                      GREEN_MSP | GREEN_MMSP | GREEN_LSP |
                      BLUE_MSP | BLUE_MMSP | BLUE_LSP);
        GPIO_BRR(GPIO_COLOR_PORT) = 0xffff; // reset colors
}

void setup_video(void) {
        // good for debugging pixel sizes
//        for (uint16_t i = 0; i < BUFFER_HEIGHT * BUFFER_WIDTH; i++) {
//                gpu_set_pixel(buffer_a, i, i % 2);
//                if (i % 8 == 0)
//                        gpu_set_pixel(buffer_a, i, 0b110);
//                if (i % 7 == 0)
//                        gpu_set_pixel(buffer_a, i, 0b111);
//        }
        // stripes
//        for (uint16_t i = 0; i < BUFFER_HEIGHT * BUFFER_WIDTH; i++)
//                gpu_set_pixel(buffer_a, i, i % 8);
}

void start_communication(void) {
        dma_channel_reset(DMA1, DMA_CHANNEL2);
        spi_reset(SPI1);
        rcc_periph_clock_enable(RCC_DMA1);
        rcc_periph_clock_enable(RCC_SPI1);
        // SCK & MOSI
        gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO3 | GPIO5);
        gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO4); // MISO
        gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO15); // NSS
        SPI1_I2SCFGR = 0;
        spi_set_slave_mode(SPI1);
        spi_set_receive_only_mode(SPI1);
        spi_set_dff_8bit(SPI1);
        spi_set_clock_phase_1(SPI1);
        spi_enable_software_slave_management(SPI1);
        spi_set_nss_high(SPI1);
        spi_set_baudrate_prescaler(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_64);
        spi_send_msb_first(SPI1);
        spi_disable_crc(SPI1);
        spi_enable_rx_dma(SPI1);
        spi_enable(SPI1);
        nvic_set_priority(NVIC_DMA1_CHANNEL2_IRQ, 1);
        nvic_enable_irq(NVIC_DMA1_CHANNEL2_IRQ);
        dma_set_peripheral_address(DMA1, DMA_CHANNEL2, (uint32_t) &SPI1_DR);
        dma_set_memory_address(DMA1, DMA_CHANNEL2, (uint32_t) operation);
        dma_set_number_of_data(DMA1, DMA_CHANNEL2, OPERATION_LENGTH);
        dma_set_read_from_peripheral(DMA1, DMA_CHANNEL2);
        dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL2);
        dma_set_peripheral_size(DMA1, DMA_CHANNEL2, DMA_CCR_PSIZE_8BIT);
        dma_set_memory_size(DMA1, DMA_CHANNEL2, DMA_CCR_MSIZE_8BIT);
        dma_set_priority(DMA1, DMA_CHANNEL2, DMA_CCR_PL_VERY_HIGH);
        dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL2);
        dma_enable_channel(DMA1, DMA_CHANNEL2);
}

void start_video(void) {
        // timers are driven by a 72MHz clock
        // TIM1 -> Horizontal sync
        rcc_periph_clock_enable(RCC_TIM1);
        rcc_periph_reset_pulse(RST_TIM1);
        timer_enable_break_main_output(TIM1);
        timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
        // 0 = 1, so we need to subtract 1 to get the expected result.
        // tim1 is driven by rcc_apb2_frequency (72MHz)
        timer_set_prescaler(TIM1, uround(rcc_apb2_frequency / (PIXEL_CLOCK / (double) BUFFER_TO_VIDEO_RATIO)) - 1);
        timer_set_period(TIM1, H_WHOLE_LINE_PIXELS / 5); // 211 (211.2)
        timer_disable_preload(TIM1);
        timer_continuous_mode(TIM1);
        timer_set_counter(TIM1, 0);
        timer_enable_oc_preload(TIM1, TIM_OC1);
        timer_enable_oc_output(TIM1, TIM_OC1);
        timer_set_oc_mode(TIM1, TIM_OC1, TIM_OCM_PWM1);
        // This OC is responsible for the PWM signal, we are high until we reach OC1.
        timer_set_oc_value(TIM1, TIM_OC1, H_SYNC_PULSE_PIXELS / BUFFER_TO_VIDEO_RATIO); // 25 (25.6)
        if (H_SYNC_POLARITY) timer_set_oc_polarity_high(TIM1, TIM_OC1);
        else timer_set_oc_polarity_low(TIM1, TIM_OC1);
        nvic_enable_irq(NVIC_TIM1_CC_IRQ);
        nvic_set_priority(NVIC_TIM1_CC_IRQ, 0);
        // OC2 will let us know when we need to start preparing and outputting the image
        timer_set_oc_value(TIM1, TIM_OC2, PREPARE_DISPLAY);
        timer_enable_irq(TIM1, TIM_DIER_CC2IE);

        // TIM3 -> Vertical sync
        rcc_periph_clock_enable(RCC_TIM3);
        rcc_periph_reset_pulse(RST_TIM3);
        timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
        // +6 is trial and error, because our hsync is not perfect we also need to change the vsync up a bit.
        timer_set_prescaler(TIM3, uround(rcc_apb2_frequency / (PIXEL_CLOCK / (double) H_WHOLE_LINE_PIXELS)) + 6);
        timer_set_period(TIM3, V_WHOLE_FRAME_LINES - 1);
        timer_disable_preload(TIM3);
        timer_continuous_mode(TIM3);
        timer_set_oc_value(TIM3, TIM_OC3, V_SYNC_PULSE_LINES);
        timer_set_counter(TIM3, 0);
        timer_enable_oc_preload(TIM3, TIM_OC3);
        timer_enable_oc_output(TIM3, TIM_OC3);
        timer_set_oc_mode(TIM3, TIM_OC3, TIM_OCM_PWM1);
        if (V_SYNC_POLARITY) timer_set_oc_polarity_high(TIM3, TIM_OC3);
        else timer_set_oc_polarity_low(TIM3, TIM_OC3);

        // enable both counters
        timer_enable_counter(TIM3);
        timer_enable_counter(TIM1);
}

void tim2_isr(void) {
        TIM_SR(TIM2) = 0x0000;
}

void __attribute__ ((optimize("O3"))) tim1_cc_isr(void) {
        if (TIM3_CNT > V_BACK_PORCH_LINES + V_SYNC_PULSE_LINES - 2 &&
            TIM3_CNT < V_BACK_PORCH_LINES + V_SYNC_PULSE_LINES + V_DISPLAY_LINES - 2) {
                buffer_line = ((TIM3_CNT - V_SYNC_PULSE_LINES - V_BACK_PORCH_LINES + 2) / 5);
                line = (const void *) front_buffer + (buffer_line * (BUFFER_WIDTH / 8) * BUFFER_BPP);

#               include "scanline.g.c"

                // some monitors sample their black voltage level in the back porch, so we need to stop outputting color
                GPIO_BRR(GPIO_COLOR_PORT) = 0xffff;
        }
        TIM_SR(TIM1) = 0x0000;
}

void invalid_operation(uint8_t *invalid_op) {
        color_palette[0] = get_port_config_for_color(0b000, 0b000, 0b111);
        color_palette[1] = get_port_config_for_color(0b111, 0b111, 0b111);
        for (uint16_t i = 0; i < 7200; ++i) {
                front_buffer[i] = error[i];
        }

        write(2, 80, 1, 0, "-UNIMPLEMENTED  OPERATION-");
        char *operation_string = malloc(sizeof(char) * 24 + 1); // 24chars + NUL
        sprintf(
                operation_string,
                "%02x %02x %02x %02x  %02x %02x %02x %02x",
                invalid_op[0],
                invalid_op[1],
                invalid_op[2],
                invalid_op[3],
                invalid_op[4],
                invalid_op[5],
                invalid_op[6],
                invalid_op[7]);
        write(8, 96, 1, 0, operation_string);
}

void dma1_channel2_isr(void) {
        invalid_operation(operation);
}