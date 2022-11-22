#include <malloc.h>
#include <stdio.h>
#include "sdcard.h"
#include "libopencm3/stm32/spi.h"
#include "libopencm3/stm32/rcc.h"
#include "gpu.h"
#include "time.h"

uint8_t sdcard_calculate_crc(const uint8_t *data, uint32_t len) {
        uint8_t crc = 0;
        for (uint32_t i = 0; i < len; i++) {
                crc ^= data[i];
                for (int j = 0; j < 8; j++) {
                        crc = (crc & 0x80u) ? ((crc << 1) ^ (SDCARD_DRIVER_CRC_POLY << 1)) : (crc << 1);
                }
        }
        return crc + 1; // setting the end bit
}

uint32_t sdcard_args_send_if_cond(bool pcie_1v2, bool pcie_avail, enum SDVoltageSupplied vhs, uint8_t pattern) {
        return 0x00000000 | (pcie_1v2 << 13) | (pcie_avail << 12) | (vhs << 8) | pattern;
}

uint8_t sdcard_transieve(uint8_t data) {
        while (SPI2_SR & SPI_SR_BSY);
        SPI2_DR = data;
        while (!(SPI2_SR & SPI_SR_RXNE));
        return SPI2_DR;
}

void sdcard_select(void) {
        sdcard_transieve(0xff);
        gpio_clear(SDCARD_DRIVER_SPI_PORT, SDCARD_DRIVER_SPI_NSS);
        sdcard_transieve(0xff);
}

void sdcard_release(void) {
        sdcard_transieve(0xff);
        gpio_set(SDCARD_DRIVER_SPI_PORT, SDCARD_DRIVER_SPI_NSS);
        sdcard_transieve(0xff);
}

void sdcard_boot_sequence(void) {
        sdcard_release();
        // MOSI needs to be high for at least 74 cycles to enter SPI mode.
        // We send 10 bytes = 80 cycles.
        for (uint8_t i = 0; i < 10; ++i) {
                sdcard_transieve(0xff);
        }
        while (SPI2_SR & SPI_SR_BSY); // wait for the last 8 bits to be sent.
}

void sdcard_send_blocking(const uint8_t *data, uint32_t len) {
        for (uint32_t i = 0; i < len; ++i) {
                sdcard_transieve(data[i]);
        }
}

uint8_t sdcard_read(void) {
        while (SPI2_SR & SPI_SR_BSY);
        if (SPI2_SR & SPI_SR_RXNE) {
                return SPI2_DR;
        }
        SPI2_DR = 0xff;
        return sdcard_read();
}

void sdcard_read_buf(uint8_t *buf, uint32_t len) {
        for (uint32_t i = 0; i < len; ++i) {
                buf[i] = sdcard_read();
        }
}

void block_while_spi_busy(void) {
        while (SPI2_SR & SPI_SR_BSY);
}

uint16_t sdcard_read_fe(uint8_t *buf, uint32_t len, uint32_t bytes_timeout) {
        uint8_t cursor;
        uint16_t crc = 0;
        while ((cursor = sdcard_transieve(0xff)) == 0xff) {
                bytes_timeout--;
                if (bytes_timeout == 0) break;
        }
        if (cursor == 0xfe) {
                for (uint32_t i = 0; i < len; ++i) {
                        *buf++ = sdcard_transieve(0xff);
                }
                // 16bit crc
                crc = sdcard_transieve(0xff) << 8;
                crc |= sdcard_transieve(0xff);
        }
        sdcard_release();
        return crc;
}

uint8_t sdcard_send_command_blocking(uint8_t cmd, uint32_t args, uint32_t bytes_timeout) {
        uint8_t send[6];
        send[0] = cmd | SD_START_BITS;
        send[1] = (args >> 24) & 0xff;
        send[2] = (args >> 16) & 0xff;
        send[3] = (args >> 8) & 0xff;
        send[4] = args & 0xff;
        send[5] = sdcard_calculate_crc(send, sizeof(send) - 1);
        sdcard_select();
        sdcard_send_blocking(send, sizeof(send));
        uint8_t response;
        while ((response = sdcard_transieve(0xff)) == 0xff) {
                bytes_timeout--;
                if (bytes_timeout == 0) break;
        }
        return response;
}

uint8_t sdcard_send_app_command_blocking(uint8_t cmd, uint32_t args, uint32_t bytes_timeout) {
        sdcard_send_command_blocking(SD_CMD55_APP_CMD, 0x00000000, 2);
        sdcard_release();
        return sdcard_send_command_blocking(cmd, args, bytes_timeout);
}

void sdcard_establish_spi(uint32_t br) {
        rcc_periph_clock_enable(RCC_SPI2);
        gpio_set_mode(SDCARD_DRIVER_SPI_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                      SDCARD_DRIVER_SPI_SCK | SDCARD_DRIVER_SPI_MOSI);
        gpio_set_mode(SDCARD_DRIVER_SPI_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, SDCARD_DRIVER_SPI_MISO);
        gpio_set_mode(SDCARD_DRIVER_SPI_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                      SDCARD_DRIVER_SPI_NSS);
        spi_init_master(SPI2, br, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE,
                        SPI_CR1_CPHA_CLK_TRANSITION_2, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
        spi_enable_software_slave_management(SPI2);
        spi_set_nss_high(SPI2);
        spi_enable(SPI2);
}

void sdcard_set_spi_baudrate(uint32_t br) {
        block_while_spi_busy();
        spi_disable(SPI2);
        spi_reset(SPI2);
        spi_init_master(SPI2, br, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE,
                        SPI_CR1_CPHA_CLK_TRANSITION_2, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
        spi_set_nss_high(SPI2);
        spi_enable(SPI2);
}

uint16_t sdcard_request_csd(uint8_t *csd) {
        union SDResponse1 r1;
        r1.repr = sdcard_send_command_blocking(SD_CMD9_SEND_CSD, 0x00000000, 8);
        if (r1.invalid) return 0;
        return sdcard_read_fe(csd, 16, 8);
}

uint32_t sdcard_calculate_size_csdv1(const uint8_t *csd) {
        uint32_t mult = 2 << (SD_CSDV1_C_SIZE_MULT(csd) + 1);
        uint32_t block_nr = (SD_CSDV1_C_SIZE(csd) + 1) * mult;
        uint32_t block_len = 2 << (SD_CSDV1_READ_BL_LEN(csd) - 1);
        return block_nr * block_len;
}

uint32_t sdcard_calculate_size_csdv2(const uint8_t *csd) {
        return 512 * (SD_CSDV2_C_SIZE(csd) + 1);
}


enum SDInitResult sdcard_init(void) {
        // sd needs to be powered for around 1ms before starting.
        sdcard_boot_sequence();
        union SDResponse1 r1;
        r1.repr = sdcard_send_command_blocking(SD_CMD0_GO_IDLE_STATE, 0x00000000, 2);
        if (r1.invalid) return SD_CARD_TIMEOUT; // gets returned when to sd failed to answer for 16 cycles
        if (!r1.in_idle_state) return SD_CARD_RESET_ERROR;
        sdcard_release();
        gpu_print_text(0, 0, 1, 0, "SD-Card detected!");
        r1.repr = sdcard_send_command_blocking(
                SD_CMD8_SEND_IF_COND,
                sdcard_args_send_if_cond(false, false, VOLTAGE_2V7_TO_3V6, SD_CHECK_PATTERN),
                2
        );
        if (r1.invalid) return SD_CARD_GENERIC_COMMUNICATION_ERROR;
        if (r1.illegal_command) return SD_CARD_GENERIC_COMMUNICATION_ERROR; // TODO: test with SD-Card prior to 2.00
        uint8_t sd_status[4];
        sdcard_read_buf(sd_status, sizeof(sd_status));
        const bool pcie_1v2 = (sd_status[2] >> 4) & 2;
        const bool pcie = (sd_status[2] >> 4) & 1;
        const enum SDVoltageSupplied vhs = sd_status[2] & 0xf;
        const uint8_t pattern = sd_status[3];
        if (pattern != SD_CHECK_PATTERN) return SD_CARD_GENERIC_COMMUNICATION_ERROR;
        if (vhs != VOLTAGE_2V7_TO_3V6) return SD_CARD_TARGET_VOLTAGE_UNSUPPORTED;
        if (pcie) return SD_CARD_GENERIC_COMMUNICATION_ERROR;
        if (pcie_1v2) return SD_CARD_GENERIC_COMMUNICATION_ERROR;
        gpu_print_text(106, 0, 1, 0, "3V3+-10%");
        sdcard_release();
        r1.repr = sdcard_send_command_blocking(SD_CMD58_READ_OCR, 0x00000000, 2);
        if (r1.invalid) return SD_CARD_GENERIC_COMMUNICATION_ERROR;
        uint8_t ocr_register[4];
        sdcard_read_buf(ocr_register, sizeof(ocr_register));
        char *text = malloc(27);
        sprintf(text, "OCR %02x %02x %02x %02x", ocr_register[0], ocr_register[1], ocr_register[2], ocr_register[3]);
        gpu_print_text(0, 8, 1, 0, text);
        sdcard_release();
        uint32_t start_time = millis_since_boot();
        while (r1.in_idle_state) {
                r1.repr = sdcard_send_app_command_blocking(SD_ACMD41_SD_SEND_OP_COND, 0x40000000, 2);
                sdcard_release();
                block(10);
                if (millis_since_boot() > start_time + 1000) break;
        }
        if (r1.in_idle_state) {
                return SD_CARD_WAKEUP_TIMEOUT;
        }
        sdcard_release();
        r1.repr = sdcard_send_command_blocking(SD_CMD58_READ_OCR, 0x00000000, 2);
        if (r1.invalid) return SD_CARD_GENERIC_COMMUNICATION_ERROR;
        sdcard_read_buf(ocr_register, sizeof(ocr_register));
        sprintf(text, "OCR: %02x %02x %02x %02x", ocr_register[0], ocr_register[1], ocr_register[2], ocr_register[3]);
        gpu_print_text(0, 8, 1, 0, text);
        if (SD_OCR_IS_SDHC_OR_SDXC(ocr_register)) {
                gpu_print_text(106, 8, 3, 0, "SDHC/SDXC");
        } else {
                gpu_print_text(106, 8, 1, 0, "SDSC");
        }
        return SD_CARD_NO_ERROR;
}

bool sdcard_init_peripheral(void) {
        sdcard_establish_spi(SPI_CR1_BAUDRATE_FPCLK_DIV_256);
        enum SDInitResult res = sdcard_init();
        sdcard_set_spi_baudrate(SPI_CR1_BAUDRATE_FPCLK_DIV_4);
        char *text = malloc(27);
        sprintf(text, "sdcard_init returned: %u", res);
        gpu_print_text(0, 100, 1, 0, text);
        switch (res) {
                case SD_CARD_TIMEOUT:
                        gpu_print_text(0, 108, 2, 0, "SD-CARD TIMEOUT");
                        return false;
                case SD_CARD_RESET_ERROR:
                        gpu_print_text(0, 108, 2, 0, "SD-CARD RESET ERROR");
                        return false;
                case SD_CARD_GENERIC_COMMUNICATION_ERROR:
                        gpu_print_text(0, 108, 2, 0, "SD-CARD GENERIC ERROR");
                        return false;
                case SD_CARD_TARGET_VOLTAGE_UNSUPPORTED:
                        gpu_print_text(0, 108, 2, 0, "SD-CARD VOLTAGE ERROR");
                        return false;
                case SD_CARD_WAKEUP_TIMEOUT:
                        gpu_print_text(0, 108, 2, 0, "SD-CARD WAKEUP TIMEOUT");
                        return false;
                default:
                        break;
        }
        uint8_t csd[16];
        sdcard_request_csd(csd);
        sprintf(text, "CSD 0-7  %02x%02x%02x%02x %02x%02x%02x%02x",
                csd[0], csd[1], csd[2], csd[3], csd[4], csd[5], csd[6], csd[7]);
        gpu_print_text(0, 16, 1, 0, text);
        gpu_block_until_ack();
        sprintf(text, "    8-15 %02x%02x%02x%02x %02x%02x%02x%02x",
                csd[8], csd[9], csd[10], csd[11], csd[12], csd[13], csd[14], csd[15]);
        gpu_print_text(0, 24, 1, 0, text);
        gpu_block_until_ack();
        sprintf(text, "S%02x", SD_CSDV2_TRAN_SPEED(csd));
        gpu_print_text(0, 24, SD_CSDV2_TRAN_SPEED(csd) == 0x32 ? 5 : 3, 0, text);
        gpu_block_until_ack();
        sprintf(text, "CAPACITY %lukb", sdcard_calculate_size_csdv2(csd));
        gpu_print_text(0, 32, 1, 0, text);
        return true;
}

void __attribute__((weak)) sdcard_on_insert(void) {
        // NOP
}

void __attribute__((weak)) sdcard_on_eject(void) {
        // NOP
}