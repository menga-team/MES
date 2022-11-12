#ifndef MES_GPU_H
#define MES_GPU_H

#include "stdint.h"

extern bool spi_dma_transmit_ongoing;

typedef struct Operation Operation;

/// The operation is a 8 byte-long sequence which tells the GPU what to do.
/// Use the `gpu_operation` methods to generate a operation.
struct Operation {
    uint8_t _0;
    uint8_t _1;
    uint8_t _2;
    uint8_t _3;
    uint8_t _4;
    uint8_t _5;
    uint8_t _6;
    uint8_t _7;
};

/// @brief Initiates the communication between CPU and GPU.
/// @return 00 00 00 ff 00 00 00 00
Operation gpu_operation_init(void);

/// @brief Inserts a buffer sx*sy at xx*yy.
/// @note When all parameters are 0 the GPU expects the whole video buffer. 160*120@3bpp (7200 bytes)
/// @attention When sending the whole buffer, use 0 for all parameters. This operation is much faster.
/// @related See `display_buf` if you want to display the buffer after sending it.
/// @return 00 00 00 00 XX YY SX SY
Operation gpu_operation_send_buf(uint8_t xx, uint8_t yy, uint8_t sx, uint8_t sy);

/// @brief Halt execution of the programm and send data to GPU.
void gpu_send_blocking(uint8_t *data, uint32_t len);

/// @brief Continue execution of the programm and send data to the GPU in the background.
/// @details Will set `spi_dma_transmit_ongoing` to true while the transmission is ongoing,
///             after finishing it will fire a interrupt and set it to false.
/// @note This method will block while `spi_dma_transmit_ongoing` is true.
void gpu_send_dma(uint32_t adr, uint32_t len);

/// Setup SPI + DMA for GPU communication.
void gpu_initiate_communication(void);
/// Send INIT every second and wait for GPU HIGH.
void gpu_block_until_ready(void);

#endif //MES_GPU_H
