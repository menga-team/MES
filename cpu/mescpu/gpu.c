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

#include "gpu.h"
#include "gpu_internal.h"
#include "timer.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <stdint.h>

volatile bool spi_dma_transmit_ongoing = false;
volatile Queue current_operation;

Operation gpu_operation_init(void) {
    return (Operation){0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00};
}

Operation gpu_operation_send_buf(Buffer buffer, uint8_t xx, uint8_t yy,
                                 uint8_t sx, uint8_t sy) {
    return (Operation){0x00, 0x00, buffer, 0x00, xx, yy, sx, sy};
}

Operation gpu_operation_display_buf(uint8_t xx, uint8_t yy, uint8_t sx,
                                    uint8_t sy) {
    return (Operation){0x00, 0x00, 0x00, 0x08, xx, yy, sx, sy};
}

Operation gpu_operation_swap_buf(void) {
    return (Operation){0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00};
}

Operation gpu_operation_print_text(Buffer buffer, uint8_t fore, uint8_t back,
                                   uint8_t size, uint8_t ox, uint8_t oy) {
    return (Operation){fore, back, buffer, 0x01, 0x00, size, ox, oy};
}

Operation gpu_operation_reset(void) {
    return (Operation){0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00};
}

Operation gpu_operation_blank(Buffer bf, uint8_t blank_with) {
    return (Operation){0x00, 0x00, bf, 0x0b, 0x00, 0x00, 0x00, blank_with};
}

Operation gpu_operation_show_startup(void) {
    return (Operation){0xff, 0xff, 0xff, 0x01, 0xff, 0xff, 0xff, 0xff};
}

Operation gpu_operation_adjust_brightness(void) {
    return (Operation){0xff, 0xff, 0xff, 0x02, 0xff, 0xff, 0xff, 0xff};
}

Operation gpu_operation_draw_sdcard(Buffer bf, uint8_t x, uint8_t y) {
    return (Operation){0xff, 0xff, bf, 0x03, 0xff, 0xff, x, y};
}

Operation gpu_operation_update_palette(void) {
    return (Operation){0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00};
}

Operation gpu_operation_draw_controller(uint8_t bf, uint8_t n, uint8_t fg,
                                        uint8_t bg, uint8_t x, uint8_t y) {
    return (Operation){0xff, bg, bf, 0x04, n, fg, x, y};
}

void gpu_print_text(Buffer buffer, uint8_t ox, uint8_t oy, uint8_t foreground,
                    uint8_t background, const char *text) {
    gpu_block_ack();
    uint8_t len = 0;
    while (text[len++] != 0)
        ;
    current_operation = (Queue){
        gpu_operation_print_text(buffer, foreground, background, len, ox, oy),
        (uint8_t *)text, len, false, false};
    gpu_send_blocking((uint8_t *)&current_operation.operation,
                      sizeof(Operation));
}

void gpu_print_text_blocking(Buffer buffer, uint8_t ox, uint8_t oy,
                             uint8_t foreground, uint8_t bg, const char *text) {
    while (!gpio_get(GPU_READY_PORT, GPU_READY))
        ;
    uint8_t len = 0;
    while (text[len++] != 0)
        ;
    Operation op =
        gpu_operation_print_text(buffer, foreground, bg, len, ox, oy);
    gpu_send_blocking((uint8_t *)&op, sizeof(Operation));
    while (gpio_get(GPU_READY_PORT, GPU_READY))
        ;
    while (!gpio_get(GPU_READY_PORT, GPU_READY))
        ;
    gpu_send_blocking((uint8_t *)text, len);
    while (gpio_get(GPU_READY_PORT, GPU_READY))
        ;
    while (!gpio_get(GPU_READY_PORT, GPU_READY))
        ;
}

void gpu_reset(void) {
    gpu_block_ack();
    current_operation = (Queue){
        gpu_operation_reset(), 0, 0, true, false,
    };
    gpu_send_blocking((uint8_t *)&current_operation.operation,
                      sizeof(Operation));
}

void gpu_send_buf(Buffer buffer, uint8_t xx, uint8_t yy, uint8_t ox, uint8_t oy,
                  void *pixels) {
    gpu_block_ack();
    Operation op = gpu_operation_send_buf(buffer, ox, oy, xx, yy);
    if (xx == 160 && yy == 120)
        op._4 = op._5 = op._6 = op._7 = 0;
    current_operation = (Queue){
        op, (uint8_t *)pixels, BUFFER_SIZE(xx, yy), false, false,
    };
    gpu_send_blocking((uint8_t *)&current_operation.operation,
                      sizeof(Operation));
}

void gpu_display_buf(uint8_t xx, uint8_t yy, uint8_t ox, uint8_t oy,
                     void *pixels) {
    gpu_block_ack();
    Operation op = gpu_operation_display_buf(ox, oy, xx, yy);
    if (xx == 160 && yy == 120)
        op._4 = op._5 = op._6 = op._7 = 0;
    current_operation = (Queue){
        op, (uint8_t *)pixels, BUFFER_SIZE(xx, yy), false, false,
    };
    gpu_send_blocking((uint8_t *)&current_operation.operation,
                      sizeof(Operation));
}

void gpu_swap_buf(void) {
    gpu_block_ack();
    current_operation = (Queue){
        gpu_operation_swap_buf(), 0, 0, true, false,
    };
    gpu_send_blocking((uint8_t *)&current_operation.operation,
                      sizeof(Operation));
}

void gpu_blank(Buffer buffer, uint8_t blank_with) {
    gpu_block_ack();
    current_operation = (Queue){
        gpu_operation_blank(buffer, blank_with), 0, 0, true, false,
    };
    gpu_send_blocking((uint8_t *)&current_operation.operation,
                      sizeof(Operation));
}

void gpu_show_startup(void) {
    gpu_block_ack();
    current_operation = (Queue){
        gpu_operation_show_startup(), 0, 0, true, false,
    };
    gpu_send_blocking((uint8_t *)&current_operation.operation,
                      sizeof(Operation));
}

void gpu_adjust_brightness(void) {
    gpu_block_ack();
    current_operation = (Queue){
        gpu_operation_adjust_brightness(), 0, 0, true, false,
    };
    gpu_send_blocking((uint8_t *)&current_operation.operation,
                      sizeof(Operation));
}

void gpu_draw_sdcard(Buffer buffer, uint8_t x, uint8_t y) {
    gpu_block_ack();
    current_operation = (Queue){
        gpu_operation_draw_sdcard(buffer, x, y), 0, 0, true, false,
    };
    gpu_send_blocking((uint8_t *)&current_operation.operation,
                      sizeof(Operation));
}

void gpu_update_palette(const uint16_t *new_palette) {
    gpu_block_ack();
    current_operation = (Queue){
        gpu_operation_update_palette(),
        (uint8_t *)new_palette,
        sizeof(uint16_t) * 8,
        false,
        false,
    };
    gpu_send_blocking((uint8_t *)&current_operation.operation,
                      sizeof(Operation));
}

void gpu_initiate_communication(void) {
    RCC_APB1ENR &= ~RCC_APB1ENR_I2C1EN; // disable i2c if it happend to be
                                        // enabled, see erata.
    AFIO_MAPR |= AFIO_MAPR_SPI1_REMAP |
                 AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON; // remap spi1 & disable JTAG.
    rcc_periph_clock_enable(RCC_SPI1);

    // SS=PA15 SCK=PB3 MISO=PB4 MOSI=PB5
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO5 | GPIO3);
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO15);
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO4);

    spi_reset(SPI1);
    spi_init_master(
        SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_2, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE,
        SPI_CR1_CPHA_CLK_TRANSITION_2, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
    spi_enable_software_slave_management(SPI1);
    spi_set_nss_high(SPI1);
    spi_enable(SPI1);

    rcc_periph_clock_enable(RCC_DMA1);
    nvic_set_priority(NVIC_DMA1_CHANNEL3_IRQ, 1);
    nvic_enable_irq(NVIC_DMA1_CHANNEL3_IRQ);

    // GPU READY
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO15);
    nvic_enable_irq(NVIC_EXTI15_10_IRQ);
    nvic_set_priority(NVIC_EXTI15_10_IRQ, 2);
    exti_select_source(EXTI15, GPIOC);
    exti_set_trigger(EXTI15, EXTI_TRIGGER_RISING);
    exti_enable_request(EXTI15);
}

void gpu_sync(void) {
    // The next line is not really needed because by the time the cpu tries to
    // sync to gpu should be already up and running, but when the cpu tries to
    // resync when resetting the gpu, GPU READY might blink, because the GPU is
    // setting up output pins. The universal solution is to just wait a bit
    // before attempting to communicate.
    timer_block_ms(10);
    Operation init = gpu_operation_init();
    current_operation = (Queue){init, 0, 0, true, false};
    while (!current_operation.ack) {
        gpio_toggle(GPIOC, GPIO13);
        gpu_send_blocking((uint8_t *)&init, sizeof(Operation));
        timer_block_ms(500);
    }
    gpio_set(GPIOC, GPIO13);
}

void gpu_send_blocking(uint8_t *data, uint32_t len) {
    for (uint32_t i = 0; i < len; ++i) {
        spi_send(SPI1, data[i]);
    }
}

void gpu_send_dma(uint32_t adr, uint32_t len) {
    if (len > 0) {
        while (spi_dma_transmit_ongoing)
            ;
        spi_dma_transmit_ongoing = true;
        dma_channel_reset(DMA1, DMA_CHANNEL3);
        dma_set_peripheral_address(DMA1, DMA_CHANNEL3, (uint32_t)&SPI1_DR);
        dma_set_memory_address(DMA1, DMA_CHANNEL3, adr);
        dma_set_number_of_data(DMA1, DMA_CHANNEL3, len);
        dma_set_read_from_memory(DMA1, DMA_CHANNEL3);
        dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL3);
        dma_set_peripheral_size(DMA1, DMA_CHANNEL3, DMA_CCR_PSIZE_8BIT);
        dma_set_memory_size(DMA1, DMA_CHANNEL3, DMA_CCR_MSIZE_8BIT);
        dma_set_priority(DMA1, DMA_CHANNEL3, DMA_CCR_PL_VERY_HIGH);
        dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL3);
        dma_enable_channel(DMA1, DMA_CHANNEL3);
        spi_enable_tx_dma(SPI1);
    }
}

void dma1_channel3_isr(void) {
    dma_clear_interrupt_flags(DMA1, DMA_CHANNEL2, DMA_ISR_TCIF_BIT);
    dma_disable_transfer_complete_interrupt(DMA1, DMA_CHANNEL3);
    spi_disable_tx_dma(SPI1);
    dma_disable_channel(DMA1, DMA_CHANNEL3);
    spi_dma_transmit_ongoing = false;
}

void gpu_block_until_ack(void) {
    gpu_block_ack();
}

void gpu_block_ack(void) {
    while (!current_operation.ack)
        ;
}

void gpu_wait_for_next_ready(void) {
    gpu_block_frame();
}

void gpu_block_frame(void) {
    current_operation.ack = false;
    gpu_block_ack();
}

void gpu_block_frames(uint32_t n) {
    while (n--) {
        gpu_block_frame();
    }
}

void gpu_reset_palette(void) {
    static const uint16_t DEFAULT_PALETTE[8] = {
        COLOR(0b000, 0b000, 0b000), COLOR(0b111, 0b111, 0b111),
        COLOR(0b111, 0b000, 0b000), COLOR(0b000, 0b111, 0b000),
        COLOR(0b000, 0b000, 0b111), COLOR(0b111, 0b111, 0b000),
        COLOR(0b111, 0b000, 0b111), COLOR(0b000, 0b111, 0b111),
    };
    gpu_update_palette(DEFAULT_PALETTE);
    gpu_block_ack();
}

void gpu_draw_controller(Buffer buffer, uint8_t n, uint8_t fg, uint8_t bg,
                         uint8_t x, uint8_t y) {
    gpu_block_ack();
    current_operation = (Queue){
        gpu_operation_draw_controller(buffer, n, fg, bg, x, y),
        0,
        0,
        true,
        false,
    };
    gpu_send_blocking((uint8_t *)&current_operation.operation,
                      sizeof(Operation));
}

void exti15_10_isr(void) {
    exti_reset_request(EXTI15);
    if (!current_operation.data_sent) {
        gpu_send_dma((uint32_t)current_operation.operation_data,
                     current_operation.operation_data_len);
        current_operation.data_sent = true;
    } else {
        current_operation.ack = true;
    }
}
