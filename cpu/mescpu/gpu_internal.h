#ifndef MES_GPU_INTERNAL_H
#define MES_GPU_INTERNAL_H

#include "stdint.h"
#include "stdbool.h"

#define GPU_READY_PORT GPIOC
#define GPU_READY GPIO15

typedef struct Operation Operation;
typedef struct Queue Queue;

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

/// The Queue holds information of the operation that is beeing currently transmitted.
struct Queue {
    Operation operation;
    uint8_t *operation_data;
    uint16_t operation_data_len;
    volatile bool data_sent;
    volatile bool ack;
};

extern volatile bool spi_dma_transmit_ongoing;
extern volatile Queue current_operation;

/// @brief Initiates the communication between CPU and GPU.
/// @return 00 00 00 ff 00 00 00 00
Operation gpu_operation_init(void);

Operation gpu_operation_swap_buf(void);

/// @brief Inserts a buffer sx*sy at xx*yy.
/// @note When all parameters are 0 the GPU expects the whole video buffer. 160*120@3bpp (7200 bytes)
/// @attention When sending the whole buffer, use 0 for all parameters. This operation is much faster.
/// @related See `display_buf` if you want to display the buffer after sending it.
/// @return 00 00 BF 00 XX YY SX SY
Operation gpu_operation_send_buf(uint8_t bf, uint8_t xx, uint8_t yy, uint8_t sx, uint8_t sy);

Operation gpu_operation_display_buf(uint8_t xx, uint8_t yy, uint8_t sx, uint8_t sy);

/// @brief Prints text of length size at ox*oy.
/// @attention Text needs to be NUL-Terminated.
/// @return FF BB BF 01  00 XX OX OY
Operation gpu_operation_print_text(uint8_t bf, uint8_t foreground, uint8_t background, uint8_t size, uint8_t ox, uint8_t oy);

/// @brief Will reset the GPU.
/// @return 00 00 00 0a 00 00 00 00
Operation gpu_operation_reset(void);

/// @brief Will fill the Buffer with the specified value.
/// @return 00 00 BF 0b 00 00 00 00
Operation gpu_operation_blank(uint8_t bf, uint8_t blank_with);

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
void gpu_sync(void);

/// Will halt execution until GPU ack operation data.
void gpu_block_until_ack(void);

/// @warning Don't use this function unless you know that you need it.
/// @related gpu_print_text
void gpu_print_text_blocking(uint8_t buffer, uint8_t ox, uint8_t oy, uint8_t foreground, uint8_t bg, const char *text);

#endif //MES_GPU_INTERNAL_H
