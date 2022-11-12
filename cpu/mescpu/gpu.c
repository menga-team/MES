#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>
#include "time.h"
#include "gpu.h"

#define GPU_READY_PORT GPIOC
#define GPU_READY GPIO15

bool spi_dma_transmit_ongoing = false;

struct Operation gpu_operation_init(void) {
        return (Operation) {0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00};
}

struct Operation gpu_operation_send_buf(uint8_t xx, uint8_t yy, uint8_t sx, uint8_t sy) {
        return (Operation) {0x00, 0x00, 0x00, 0x00, xx, yy, sx, sy};
}

void gpu_initiate_communication(void) {
        RCC_APB1ENR &= ~RCC_APB1ENR_I2C1EN; // disable i2c if it happend to be enabled, see erata.
        AFIO_MAPR |= AFIO_MAPR_SPI1_REMAP | AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON; // remap spi1 & disable JTAG.
        rcc_periph_clock_enable(RCC_SPI1);

        // SS=PA15 SCK=PB3 MISO=PB4 MOSI=PB5
        gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO5 | GPIO3);
        gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO15);
        gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO4);

        spi_reset(SPI1);
        spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_64, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE,
                        SPI_CR1_CPHA_CLK_TRANSITION_2, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
        spi_enable_software_slave_management(SPI1);
        spi_set_nss_high(SPI1);
        spi_enable(SPI1);

        rcc_periph_clock_enable(RCC_SPI1);
        nvic_set_priority(NVIC_DMA1_CHANNEL3_IRQ, 1);
        nvic_enable_irq(NVIC_DMA1_CHANNEL3_IRQ);

        // GPU READY
        nvic_enable_irq(NVIC_EXTI0_IRQ);
        gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO15);
        exti_select_source(EXTI0, GPIOC);
        exti_set_trigger(EXTI0, EXTI_TRIGGER_RISING);
        exti_enable_request(EXTI0);
}

void gpu_block_until_ready(void) {
        Operation init = gpu_operation_init();
        while (!gpio_get(GPU_READY_PORT, GPU_READY)) {
                gpio_toggle(GPIOC, GPIO13);
                gpu_send_blocking((uint8_t *) &init, 8);
                block(500);
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
                while (spi_dma_transmit_ongoing);
                spi_dma_transmit_ongoing = true;
                dma_channel_reset(DMA1, DMA_CHANNEL3);
                dma_set_peripheral_address(DMA1, DMA_CHANNEL3, (uint32_t) &SPI1_DR);
                dma_set_memory_address(DMA1, DMA_CHANNEL3, adr);
                dma_set_number_of_data(DMA1, DMA_CHANNEL3, len);
                dma_set_read_from_memory(DMA1, DMA_CHANNEL3);
                dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL3);
                dma_set_peripheral_size(DMA1, DMA_CHANNEL3, DMA_CCR_PSIZE_8BIT);
                dma_set_memory_size(DMA1, DMA_CHANNEL3, DMA_CCR_MSIZE_8BIT);
                dma_set_priority(DMA1, DMA_CHANNEL3, DMA_CCR_PL_HIGH);
                dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL3);
                dma_enable_channel(DMA1, DMA_CHANNEL3);
                spi_enable_tx_dma(SPI1);
        }
}

void dma1_channel3_isr(void) {
        dma_disable_transfer_complete_interrupt(DMA1, DMA_CHANNEL3);
        spi_disable_tx_dma(SPI1);
        dma_disable_channel(DMA1, DMA_CHANNEL3);
        spi_dma_transmit_ongoing = false;
}

void exti0_isr(void) {
        exti_reset_request(EXTI0);
}