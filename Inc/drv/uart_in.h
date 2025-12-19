/*
 * Copyright (c) 2025 FOC2 Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UART_IN_H
#define UART_IN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief UART input driver with interrupt-based reception
 *
 * This driver provides interrupt-based character reception from UART.
 * It uses a circular buffer to store received characters.
 */

#ifndef UART_IN_BUFFER_SIZE
#define UART_IN_BUFFER_SIZE 16  /* Must be power of 2 for efficiency */
#endif

/**
 * @brief Callback function type for received characters
 *
 * @param ch Received character
 */
typedef void (*uart_in_rx_callback_t)(uint8_t ch);

/**
 * @brief Initialize UART input driver
 *
 * @param huart Pointer to UART handle (e.g., &huart2)
 * @return 0 on success, negative value on failure
 */
int uart_in_init(UART_HandleTypeDef *huart);

/**
 * @brief Register callback for received characters
 *
 * @param callback Callback function to be called when character is received
 */
void uart_in_set_callback(uart_in_rx_callback_t callback);

/**
 * @brief Check if data is available in the receive buffer
 *
 * @return Number of bytes available
 */
uint32_t uart_in_available(void);

/**
 * @brief Read one character from the receive buffer
 *
 * @param ch Pointer to store the received character
 * @return true if character was read, false if buffer is empty
 */
bool uart_in_getchar(uint8_t *ch);

/**
 * @brief Read multiple characters from the receive buffer
 *
 * @param buf Buffer to store received characters
 * @param len Maximum number of characters to read
 * @return Number of characters actually read
 */
uint32_t uart_in_read(uint8_t *buf, uint32_t len);

/**
 * @brief Clear the receive buffer
 */
void uart_in_flush(void);

/**
 * @brief UART interrupt callback (called from HAL_UART_RxCpltCallback)
 *
 * This function should be called from the HAL UART RX complete callback.
 * It handles the received character and restarts reception.
 *
 * @param huart Pointer to UART handle
 */
void uart_in_irq_handler(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif /* UART_IN_H */
