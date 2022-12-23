#include "main.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/dma.h>
#include "libopencm3/cm3/systick.h"
#include <mesmath.h>
#include <stdio.h>
#include "gpu.h"

// Include error screen as 'error'
#include "images/error.m3ifc"
// Include cpu timeout screen as 'cpu_timeout'
#include "images/cpu_timeout.m3ifc"

int main(void) {
        rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
        setup_output();
        gpu_init();
        setup_video();
        start_communication();
        start_video();
        // Setup timeout
        systick_clear();
        systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);
        systick_set_reload(16777215); // 24-bit limit. (~1.8s)
        systick_interrupt_enable();
        systick_counter_enable();
        while (run) {
                // we have ~5µs every line and ~770µs every frame
                handle_operation();
        }
        while (true);
}

void sys_tick_handler(void) {
        systick_interrupt_disable();
        systick_counter_disable();
        systick_clear();
        // Did we sync after ~1.8s?
        // If we didn't then show the timeout error.
        if(processing_stage == UNINITIALIZED) {
                // Show timeout screen if cpu failed to sync.
                cpu_communication_timeout();
                // Run gets disabled when the 1. operation sent is not the init operation.
                // Inorder to help debugging we will display the "COM_BRK" error, meaning the gpu is able to recieve
                // from the cpu, but the communication might be unreliable.
                if(!run) {
                        gpu_write(front_buffer, 118, 112, 2, 3, "COM_BRK");
                }
        }
}

void setup_output(void) {
        rcc_periph_clock_enable(RCC_AFIO);
        rcc_periph_clock_enable(RCC_GPIOA);
        rcc_periph_clock_enable(RCC_GPIOB);
        rcc_periph_clock_enable(RCC_GPIOC);
        gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13); // built-in
        gpio_set_mode(GPU_READY_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPU_READY);
        gpio_set(GPIOC, GPIO13);
        // A8: hsync (yellow cable)
        // B0: vsync (orange cable)
        gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_TIM1_CH1); // PA8
        gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_TIM3_CH3); // PB0
        gpio_set_mode(GPIO_COLOR_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                      RED_MSP | RED_MMSP | RED_LSP |
                      GREEN_MSP | GREEN_MMSP | GREEN_LSP |
                      BLUE_MSP | BLUE_MMSP | BLUE_LSP);
        gpio_clear(GPIO_COLOR_PORT, GPIO_ALL); // reset colors
        gpio_clear(GPU_READY_PORT, GPU_READY); // set gpu ready to low
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
        for (uint16_t i = 0; i < 7200; i++)
                front_buffer[i] = 0x00;
}

void start_communication(void) {
        RCC_APB1ENR &= ~RCC_APB1ENR_I2C1EN; // disable i2c if it happend to be enabled, see erata.
        // SS=PA4 SCK=PA5 MISO=PA6 MOSI=PA7
        gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO6);
        gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO5 | GPIO4 | GPIO7);
        rcc_periph_clock_enable(RCC_SPI1);
        spi_reset(SPI1);
        spi_set_slave_mode(SPI1);
        spi_set_clock_polarity_1(SPI1);
        spi_set_clock_phase_1(SPI1);
        spi_set_dff_8bit(SPI1);
        spi_send_msb_first(SPI1);
        spi_enable(SPI1);
        rcc_periph_clock_enable(RCC_DMA1);
        nvic_set_priority(NVIC_DMA1_CHANNEL2_IRQ, 1);
        nvic_enable_irq(NVIC_DMA1_CHANNEL2_IRQ);
        dma_recieve_operation();
}

void dma_recieve_operation(void) {
        dma_recieve((uint32_t) &operation, OPERATION_LENGTH);
}

void dma_recieve(uint32_t adr, uint32_t len) {
        dma_channel_reset(DMA1, DMA_CHANNEL2);
        dma_set_peripheral_address(DMA1, DMA_CHANNEL2, (uint32_t) &SPI1_DR);
        dma_set_memory_address(DMA1, DMA_CHANNEL2, adr);
        dma_set_number_of_data(DMA1, DMA_CHANNEL2, len);
        dma_set_read_from_peripheral(DMA1, DMA_CHANNEL2);
        dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL2);
        dma_set_peripheral_size(DMA1, DMA_CHANNEL2, DMA_CCR_PSIZE_8BIT);
        dma_set_memory_size(DMA1, DMA_CHANNEL2, DMA_CCR_MSIZE_8BIT);
        dma_set_priority(DMA1, DMA_CHANNEL2, DMA_CCR_PL_VERY_HIGH);
        dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL2);
        spi_enable_rx_dma(SPI1);
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
                GPIO_BRR(GPIO_COLOR_PORT) = GPIO_ALL;
        }
        TIM_SR(TIM1) = 0x0000;
}

void generic_error(void) {
        run = false;
        color_palette[0] = COLOR(0b000, 0b000, 0b111);
        color_palette[1] = COLOR(0b111, 0b111, 0b111);
        for (uint16_t i = 0; i < 7200; ++i) {
                front_buffer[i] = error[i];
        }
}

void invalid_operation(uint8_t *invalid_op) {
        generic_error();
        gpu_write(front_buffer, 2, 80, 1, 0, "-UNIMPLEMENTED  OPERATION-");
        char *operation_string = malloc(sizeof(char) * 25); // 24chars + NUL
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
        gpu_write(front_buffer, 8, 96, 1, 0, operation_string);
}

void unexpected_data(enum Stage c_stage) {
        generic_error();
        gpu_write(front_buffer, 2, 80, 1, 0, "  -  UNEXPECTED  DATA  -  ");
        gpu_write(front_buffer, 0, 88, 1, 0, "Current processing stage:");
        char *buf = malloc(sizeof(char) * 27); // 26chars (whole line) + NUL
        sprintf(buf, "%s (%02x)", stage_pretty_names[c_stage], c_stage);
        gpu_write(front_buffer, 0, 96, 1, 0, buf);
        sprintf(buf, "Recieved data: (%04lx)", DMA_CMAR(DMA1, DMA_CHANNEL2));
        gpu_write(front_buffer, 0, 104, 1, 0, buf);
        uint8_t *data = (uint8_t *) DMA_CMAR(DMA1, DMA_CHANNEL2);
        sprintf(buf, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x ...",
                data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10]);
        gpu_write(front_buffer, 0, 112, 1, 0, buf);
}

void cpu_communication_timeout(void) {
        color_palette[0] = COLOR(0b000, 0b000, 0b000);
        color_palette[1] = COLOR(0b110, 0b110, 0b110);
        color_palette[2] = COLOR(0b100, 0b100, 0b100);
        color_palette[3] = COLOR(0b111, 0b111, 0b111);
        for (uint16_t i = 0; i < 7200; ++i) {
                front_buffer[i] = cpu_timeout[i];
        }
}

void dma1_channel2_isr(void) {
        dma_disable_transfer_complete_interrupt(DMA1, DMA_CHANNEL2);
        spi_disable_rx_dma(SPI1);
        dma_disable_channel(DMA1, DMA_CHANNEL2);
        switch (processing_stage) {
                case UNINITIALIZED:
                        if(OPERATION_ID(operation) != OPERATION_ID_INIT) {
                                // 1. Operation was not the sync operation...
                                // The communication is not reliable and should not be trusted with further operations.
                                // An error will be displayed automatically via the systick interupt.
                                run = false;
                                break;
                        }
                case READY:
                        processing_stage = UNHANDELED_OPERATION;
                        break;
                case WAITING_FOR_DATA:
                        processing_stage = UNHANDELED_DATA;
                        break;
                case WAITING_FOR_DMA:
                        processing_stage = READY;
                        GPIO_BSRR(GPU_READY_PORT) = GPU_READY;
                default:
                        unexpected_data(processing_stage);
                        break;
        }
}

void handle_operation(void) {
        switch (processing_stage) {
                case UNHANDELED_OPERATION: {
                        new_operation();
                        break;
                }
                case UNHANDELED_DATA: {
                        new_data();
                        break;
                }
                default: {
                        break;
                }
        }
}

void new_operation(void) {
        GPIO_BRR(GPU_READY_PORT) = GPU_READY;
        switch (OPERATION_ID(operation)) {
                case OPERATION_ID_INIT: {
                        gpu_write(front_buffer, 0, 0, 1, 0, "CPU synced!");
                        dma_recieve_operation();
                        processing_stage = READY;
                        break;
                }
                case OPERATION_ID_SWAP_BUF: {
                        gpu_swap_buffers();
                        dma_recieve_operation();
                        processing_stage = READY;
                        break;
                }
                case OPERATION_ID_SEND_BUF: {
                        if (operation[4] == 0x00 && operation[5] == 0x00 &&
                            operation[6] == 0x00 && operation[7] == 0x00) {
                                // recieve full frame via dma
                                dma_recieve((uint32_t) front_buffer, BUFFER_SIZE);
                                processing_stage = WAITING_FOR_DMA;
                        } else {
                                dma_recieve(
                                        (uint32_t) operation_data,
                                        uround((operation[6] * operation[7] * BUFFER_BPP) / 8)
                                );
                                processing_stage = WAITING_FOR_DATA;
                        }
                        break;
                }
                case OPERATION_ID_PRINT_TEXT: {
                        dma_recieve(
                                (uint32_t) operation_data,
                                operation[5]
                        );
                        processing_stage = WAITING_FOR_DATA;
                        break;
                }
                default: { // unimplemented operation
                        invalid_operation(operation);
                        processing_stage = READY;
                        break;
                }
        }
        GPIO_BSRR(GPU_READY_PORT) = GPU_READY;
}

void new_data(void) {
        GPIO_BRR(GPU_READY_PORT) = GPU_READY;
        switch (OPERATION_ID(operation)) {
                case OPERATION_ID_SEND_BUF: {
                        const uint8_t o_x = operation[4];
                        const uint8_t o_y = operation[5];
                        const uint8_t s_x = operation[6];
                        const uint8_t s_y = operation[7];
                        // TODO: Optimize
                        for (uint8_t c_x = 0; c_x < s_x; ++c_x) {
                                for (uint8_t c_y = 0; c_y < s_y; ++c_y) {
                                        gpu_set_pixel(
                                                front_buffer,
                                                (o_y + c_y) * BUFFER_WIDTH + o_x + c_x,
                                                gpu_get_pixel(operation_data, c_y * s_y + c_x)
                                        );
                                }
                        }
                        break;
                }
                case OPERATION_ID_PRINT_TEXT: {
                        gpu_write(front_buffer, operation[6], operation[7], operation[0], operation[1],
                                  (char *) operation_data);
                        break;
                }
        }
        dma_recieve_operation();
        processing_stage = READY;
        GPIO_BSRR(GPU_READY_PORT) = GPU_READY;
}