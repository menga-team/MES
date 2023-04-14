#include "main.h"
#include "gpu.h"
#include "libopencm3/cm3/systick.h"
#include "mesgraphics.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/timer.h>
#include <memory.h>
#include <mesmath.h>
#include <stdint.h>
#include <stdio.h>

// Include error screen as 'error'
#include "images/error.m3ifc"
// Include cpu timeout screen as 'cpu_timeout'
#include "images/cpu_timeout.m3ifc"
// The boot screen
#include "images/splash_screen.m3ifc"
// The adjust brightness screen
#include "images/adjust_brightness.m3ifc"
// Sdcard icon
#include "images/sdcard.m3ifc"

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

    while (true)
        ;
}

void sys_tick_handler(void) {
    systick_interrupt_disable();
    systick_counter_disable();
    systick_clear();
    // Did we sync after ~1.8s?
    // If we didn't then show the timeout error.
    if (processing_stage == UNINITIALIZED) {
        // Show timeout screen if cpu failed to sync.
        cpu_communication_timeout();
        // Run gets disabled when the 1. operation sent is not the init
        // operation. Inorder to help debugging we will display the
        // "COM_BRK" error, meaning the gpu is able to recieve from the
        // cpu, but the communication might be unreliable.
        if (!run) {
            gpu_write(front_buffer, 118, 112, 2, 3, "COM_BRK");
        }
    }
}

void setup_output(void) {
    rcc_periph_clock_enable(RCC_AFIO);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                  GPIO13); // built-in
    gpio_set_mode(GPU_READY_PORT, GPIO_MODE_OUTPUT_2_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPU_READY);
    gpio_clear(GPU_READY_PORT, GPU_READY); // set gpu ready to low
    gpio_set(GPIOC, GPIO13);
    // A8: hsync (yellow cable)
    // B0: vsync (orange cable)
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                  GPIO_TIM1_CH1); // PA8
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                  GPIO_TIM3_CH3); // PB0
    gpio_set_mode(GPIO_COLOR_PORT, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL,
                  RED_MSP | RED_MMSP | RED_LSP | GREEN_MSP | GREEN_MMSP |
                      GREEN_LSP | BLUE_MSP | BLUE_MMSP | BLUE_LSP);
    gpio_clear(GPIO_COLOR_PORT, GPIO_ALL); // reset colors
}

void setup_video(void) { gpu_blank(front_buffer, 0x00); }

void start_communication(void) {
    RCC_APB1ENR &= ~RCC_APB1ENR_I2C1EN; // disable i2c if it happend to be
                                        // enabled, see erata.
    // SS=PA4 SCK=PA5 MISO=PA6 MOSI=PA7
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO6);
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
                  GPIO5 | GPIO4 | GPIO7);
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
    dma_channel_reset(DMA1, DMA_CHANNEL2);
    dma_set_peripheral_address(DMA1, DMA_CHANNEL2, (uint32_t)&SPI1_DR);
    dma_set_read_from_peripheral(DMA1, DMA_CHANNEL2);
    dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL2);
    dma_set_peripheral_size(DMA1, DMA_CHANNEL2, DMA_CCR_PSIZE_8BIT);
    dma_set_memory_size(DMA1, DMA_CHANNEL2, DMA_CCR_MSIZE_8BIT);
    dma_set_priority(DMA1, DMA_CHANNEL2, DMA_CCR_PL_VERY_HIGH);
    dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL2);
    spi_enable_rx_dma(SPI1);
    dma_recieve_operation();
    gpu_ready_port = 0;
}

void dma_recieve_operation(void) {
    dma_recieve((uint32_t)&operation, OPERATION_LENGTH);
}

void dma_recieve(uint32_t adr, uint32_t len) {
    dma_set_memory_address(DMA1, DMA_CHANNEL2, adr);
    dma_set_number_of_data(DMA1, DMA_CHANNEL2, len);
    dma_enable_channel(DMA1, DMA_CHANNEL2);
}

void start_video(void) {
    // timers are driven by a 72MHz clock
    // TIM1 -> H-SYNC & scanline interrupt
    rcc_periph_clock_enable(RCC_TIM1);
    rcc_periph_reset_pulse(RST_TIM1);
    timer_enable_break_main_output(TIM1);
    timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    // 25175000/4 = 6293750
    // 72000000/6293750 = 11.44 = 11
    timer_set_prescaler(TIM1,
                        uround((double)rcc_apb2_frequency /
                               (double)(PIXEL_CLOCK / BUFFER_TO_VIDEO_RATIO)) -
                            1);
    timer_set_period(TIM1, (H_WHOLE_LINE_PIXELS / BUFFER_TO_VIDEO_RATIO) - 1);
    timer_disable_preload(TIM1);
    timer_continuous_mode(TIM1);
    timer_set_counter(TIM1, 0);
    timer_enable_oc_preload(TIM1, TIM_OC1);
    timer_enable_oc_output(TIM1, TIM_OC1);
    timer_set_oc_mode(TIM1, TIM_OC1, TIM_OCM_PWM1);
    // This OC is responsible for the horizontal sync pulse...
    // We will send the sync pulse at the beging because it is easier.
    // The backporch will follow after it, and the front porch is at the
    // end.
    timer_set_oc_value(TIM1, TIM_OC1,
                       (H_SYNC_PULSE_PIXELS / BUFFER_TO_VIDEO_RATIO) - 1);
#if H_SYNC_POLARITY == 1
    timer_set_oc_polarity_high(TIM1, TIM_OC1);
#else
    timer_set_oc_polarity_low(TIM1, TIM_OC1);
#endif
    nvic_enable_irq(NVIC_TIM1_CC_IRQ);
    nvic_set_priority(NVIC_TIM1_CC_IRQ, 0);
    // OC2 will let us know when we need to start preparing and outputting
    // the image Start a little sooner because we are doing more than just
    // outputting the pixels.
    timer_set_oc_value(
        TIM1, TIM_OC2,
        ((H_SYNC_PULSE_PIXELS + H_BACK_PORCH_PIXELS) / BUFFER_TO_VIDEO_RATIO) -
            9);
    timer_enable_irq(TIM1, TIM_DIER_CC2IE);
    // TIM3 -> V-SYNC & line count
    rcc_periph_clock_enable(RCC_TIM3);
    rcc_periph_reset_pulse(RST_TIM3);
    timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    // 25175000/4 = 6293750
    // 6293750/200 = 31468.75
    // 72000000.0/31468.75 = 228.8 = 229
    timer_set_prescaler(
        TIM3, uround((double)rcc_apb2_frequency /
                     (double)((double)(PIXEL_CLOCK / BUFFER_TO_VIDEO_RATIO) /
                              (double)(H_WHOLE_LINE_PIXELS /
                                       BUFFER_TO_VIDEO_RATIO))) -
                  1);
    timer_set_period(TIM3, V_WHOLE_FRAME_LINES - 1);
    timer_disable_preload(TIM3);
    timer_continuous_mode(TIM3);
    timer_set_oc_value(TIM3, TIM_OC3, V_SYNC_PULSE_LINES);
    timer_set_counter(TIM3, 0);
    timer_enable_oc_preload(TIM3, TIM_OC3);
    timer_enable_oc_output(TIM3, TIM_OC3);
    timer_set_oc_mode(TIM3, TIM_OC3, TIM_OCM_PWM1);
#if V_SYNC_POLARITY == 1
    timer_set_oc_polarity_high(TIM3, TIM_OC3);
#else
    timer_set_oc_polarity_low(TIM3, TIM_OC3);
#endif
    // initialize line counters
    buffer_line = 0;
    sub_line = -1;
    // start timers
    timer_enable_counter(TIM3);
    timer_enable_counter(TIM1);
}

void __attribute__((optimize("O3"))) tim1_cc_isr(void) {
    TIM_SR(TIM1) = 0x0000;
    if (TIM3_CNT > V_SYNC_PULSE_LINES + V_BACK_PORCH_LINES - 3 &&
        TIM3_CNT <
            V_SYNC_PULSE_LINES + V_BACK_PORCH_LINES + V_DISPLAY_LINES - 20) {
        // while drawing we don't want to communicate, because DMA and
        // CPU can only access memory one at a time. When we are
        // recieving data and drawing at the same time, artifacts will
        // appear.
        GPIO_BRR(GPU_READY_PORT) = GPU_READY;

        // Increment subline
        sub_line++;

        // Send the scanline
        line = (const void *)front_buffer +
               (buffer_line * (BUFFER_WIDTH / 8) * BUFFER_BPP);

#include "scanline.g.c"

        // Some monitors sample their black voltage level in the back
        // porch, so we need to stop outputting color.
        GPIO_BRR(GPIO_COLOR_PORT) = GPIO_ALL;

        // Branches are last because we don't want variable execution
        // time before sending the pixels.
        if (sub_line == BUFFER_TO_VIDEO_RATIO) {
            buffer_line++;
            sub_line = 0;
        }
        if (buffer_line == BUFFER_HEIGHT)
            buffer_line = 0;
    } else {
        // we don't want to send a possible GPU_READY after finishing a
        // line, because that's just to many interrupts.
        GPIO_ODR(GPU_READY_PORT) = gpu_ready_port;
    }
}

void generic_error(void) {
    run = false;
    gpu_ready_port = 0;
    color_palette[0] = COLOR(0b000, 0b000, 0b111);
    color_palette[1] = COLOR(0b111, 0b111, 0b111);
    memcpy(front_buffer, error, SCREEN_BUFFER_SIZE);
}

void invalid_operation(uint8_t *invalid_op) {
    generic_error();
    gpu_write(front_buffer, 2, 80, 1, 0, "-UNIMPLEMENTED  OPERATION-");
    char *operation_string = malloc(sizeof(char) * 25); // 24chars + NUL
    sprintf(operation_string, "%02x %02x %02x %02x  %02x %02x %02x %02x",
            invalid_op[0], invalid_op[1], invalid_op[2], invalid_op[3],
            invalid_op[4], invalid_op[5], invalid_op[6], invalid_op[7]);
    gpu_write(front_buffer, 8, 96, 1, 0, operation_string);
    free(operation_string);
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
    uint8_t *data = (uint8_t *)DMA_CMAR(DMA1, DMA_CHANNEL2);
    sprintf(buf, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x ...", data[0],
            data[1], data[2], data[3], data[4], data[5], data[6], data[7],
            data[8], data[9], data[10]);
    gpu_write(front_buffer, 0, 112, 1, 0, buf);
    free(buf);
}

void too_large_data(uint16_t bytes) {
    generic_error();
    gpu_write(front_buffer, 2, 80, 1, 0, "-OPERATION DATA TOO LARGE-");
    char *buf = malloc(sizeof(char) * 27); // 26chars (whole line) + NUL
    sprintf(buf, "%u > %u!", bytes, OPERATION_DATA_LENGTH);
    gpu_write(front_buffer, 2, 88, 1, 0, buf);
    gpu_write(front_buffer, 2, 96, 1, 0, "OPERATION:");
    sprintf(buf, "%02x %02x %02x %02x  %02x %02x %02x %02x", operation[0],
            operation[1], operation[2], operation[3], operation[4],
            operation[5], operation[6], operation[7]);
    gpu_write(front_buffer, 2, 104, 1, 0, buf);
    free(buf);
}

void cpu_communication_timeout(void) {
    // FIXME: When a timeout is displayed the lines are in the wrong order.
    //      This is probably because the pixels get copied in a interrupt.
    //      A solution could be to copy the pixels via DMA or to copy the
    //      pixels outside of the interrupt.
    color_palette[0] = COLOR(0b000, 0b000, 0b000);
    color_palette[1] = COLOR(0b110, 0b110, 0b110);
    color_palette[2] = COLOR(0b100, 0b100, 0b100);
    color_palette[3] = COLOR(0b111, 0b111, 0b111);
    memcpy(front_buffer, cpu_timeout, SCREEN_BUFFER_SIZE);
}

void dma1_channel2_isr(void) {
    dma_clear_interrupt_flags(DMA1, DMA_CHANNEL2, DMA_ISR_TCIF_BIT);
    dma_disable_channel(DMA1, DMA_CHANNEL2);
    switch (processing_stage) {
    case UNINITIALIZED:
        if (OPERATION_ID(operation) != OPERATION_ID_INIT) {
            // 1. Operation was not the sync operation...
            // The communication is not reliable and should not be
            // trusted with further operations. An error will be
            // displayed automatically via the systick interupt.
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
        dma_recieve_operation();
        GPIO_BSRR(GPU_READY_PORT) = GPU_READY;
        gpu_ready_port = GPU_READY;
        break;
    default:
        unexpected_data(processing_stage);
        break;
    }
}

void handle_operation(void) {
    switch (processing_stage) {
    case UNHANDELED_OPERATION:
        new_operation();
        break;
    case UNHANDELED_DATA:
        new_data();
        break;
    default:
        break;
    }
}

void new_operation(void) {
    gpu_ready_port = 0;
    GPIO_BRR(GPU_READY_PORT) = GPU_READY;
    if (INTERNAL_OPERATION(operation)) {

        switch (OPERATION_ID(operation)) {
        case INTERNAL_SHOW_STARTUP:
            memcpy(front_buffer, splash_screen, SCREEN_BUFFER_SIZE);
            uint8_t su_colors[4][3] = {
                {0b000, 0b000, 0b000},
                {0b111, 0b001, 0b001},
                {0b101, 0b101, 0b101},
                {0b111, 0b111, 0b111},
            };

            // set the inital palette
            for (uint8_t i = 0; i < 4; ++i)
                color_palette[i] =
                    COLOR(su_colors[i][0], su_colors[i][1], su_colors[i][2]);

            // not going to start a timer just for that...
            for (uint32_t i = 0; i < 5000000; ++i)
                __asm__("nop");

            // fade out
            while (su_colors[3][0] != 0) { // [3] is white
                for (uint8_t i = 0; i < 4; ++i) {
                    for (uint8_t j = 0; j < 3; ++j) {
                        if (su_colors[i][j] != 0)
                            su_colors[i][j]--;
                    }
                    color_palette[i] = COLOR(su_colors[i][0], su_colors[i][1],
                                             su_colors[i][2]);
                }
                for (uint32_t i = 0; i < 200000; ++i)
                    __asm__("nop");
            }
            gpu_blank(front_buffer, 0x00);
            gpu_reset_palette();
            break;
        case INTERNAL_ADJUST_BRIGHTNESS:
            memcpy(front_buffer, adjust_brightness, SCREEN_BUFFER_SIZE);
            uint8_t ab_colors[8][3] = {
                {0b000, 0b000, 0b000}, {0b110, 0b110, 0b110},
                {0b101, 0b101, 0b101}, {0b111, 0b111, 0b111},
                {0b100, 0b100, 0b100}, {0b011, 0b011, 0b011},
                {0b010, 0b010, 0b010}, {0b001, 0b001, 0b001},
            };

            // set the inital palette
            for (uint8_t i = 0; i < 8; ++i)
                color_palette[i] =
                    COLOR(ab_colors[i][0], ab_colors[i][1], ab_colors[i][2]);

            for (uint32_t i = 0; i < 10000000; ++i)
                __asm__("nop");

            // fade out
            while (ab_colors[3][0] != 0) { // [3] is white
                for (uint8_t i = 0; i < 8; ++i) {
                    for (uint8_t j = 0; j < 3; ++j) {
                        if (ab_colors[i][j] != 0)
                            ab_colors[i][j]--;
                    }
                    color_palette[i] = COLOR(ab_colors[i][0], ab_colors[i][1],
                                             ab_colors[i][2]);
                }
                for (uint32_t i = 0; i < 300000; ++i)
                    __asm__("nop");
            }
            gpu_blank(front_buffer, 0x00);
            gpu_reset_palette();
            break;
        case INTERNAL_DRAW_SDCARD:
            gpu_insert_buf(OPERATION_BUFFER(operation), sdcard, SDCARD_WIDTH,
                           SDCARD_HEIGHT, operation[6], operation[7]);
            break;
        default:
            invalid_operation(operation);
            break;
        }

        dma_recieve_operation();
        processing_stage = READY;
        gpu_ready_port = GPU_READY;
        return;
    }
    switch (OPERATION_ID(operation)) {
    case OPERATION_ID_INIT:
        gpu_write(front_buffer, 0, 0, 1, 0, "GPU INITIALIZED");
        dma_recieve_operation();
        processing_stage = READY;
        break;
    case OPERATION_ID_SWAP_BUF:
        gpu_swap_buffers();
        dma_recieve_operation();
        processing_stage = READY;
        break;
    case OPERATION_ID_DISPLAY_BUF:
    case OPERATION_ID_SEND_BUF:
        if (operation[4] == 0x00 && operation[5] == 0x00 &&
            operation[6] == 0x00 && operation[7] == 0x00) {
            if (OPERATION_ID(operation) ==
                OPERATION_ID_DISPLAY_BUF) { // TODO: stupid hack...
                operation[2] = 0x01;
            }
            // recieve full frame via dma
            dma_recieve((uint32_t)OPERATION_BUFFER(operation),
                        SCREEN_BUFFER_SIZE);
            processing_stage = WAITING_FOR_DMA;
        } else {
            uint16_t buffer_size = BUFFER_SIZE(operation[6], operation[7]);
            if (buffer_size > OPERATION_DATA_LENGTH) {
                too_large_data(buffer_size);
                return;
            }
            dma_recieve((uint32_t)operation_data, buffer_size);
            processing_stage = WAITING_FOR_DATA;
        }
        GPIO_BSRR(GPU_READY_PORT) = GPU_READY;
        return;
    case OPERATION_ID_PRINT_TEXT:
        dma_recieve((uint32_t)operation_data, operation[5]);
        processing_stage = WAITING_FOR_DATA;
        break;
    case OPERATION_ID_CHANGE_PALETTE:
        dma_recieve((uint32_t)color_palette, sizeof(color_palette));
        processing_stage = WAITING_FOR_DMA;
        break;
    case OPERATION_ID_RESET:
        gpu_reset();
        // won't be executed. vvvv
        dma_recieve_operation();
        processing_stage = READY;
        break;
    case OPERATION_ID_BLANK:
        gpu_blank(OPERATION_BUFFER(operation), operation[7]);
        dma_recieve_operation();
        processing_stage = READY;
        break;
    default: // unimplemented operation
        invalid_operation(operation);
        processing_stage = READY;
        break;
    }
    gpu_ready_port = GPU_READY;
}

void new_data(void) {
    gpu_ready_port = 0;
    GPIO_BRR(GPU_READY_PORT) = GPU_READY;
    switch (OPERATION_ID(operation)) {
    case OPERATION_ID_DISPLAY_BUF:
    case OPERATION_ID_SEND_BUF: {
        const uint8_t x = operation[4];
        const uint8_t y = operation[5];
        const uint8_t width = operation[6];
        const uint8_t height = operation[7];
        gpu_insert_buf(OPERATION_BUFFER(operation), operation_data, width,
                       height, x, y);
        if (OPERATION_ID(operation) == OPERATION_ID_DISPLAY_BUF) {
            gpu_swap_buffers();
        }
        break;
    }
    case OPERATION_ID_PRINT_TEXT:
        gpu_write(OPERATION_BUFFER(operation), operation[6], operation[7],
                  operation[0], operation[1], (char *)operation_data);
        break;
    }
    dma_recieve_operation();
    processing_stage = READY;
    gpu_ready_port = GPU_READY;
}
