/*
 * Copyright (c) 2025 FOC2 Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "drv/uart_in.h"
#include <stdio.h>
#include <string.h>

/* Circular buffer for received characters */
static uint8_t rx_buffer[UART_IN_BUFFER_SIZE];
static volatile uint32_t rx_head = 0;  /* Write index */
static volatile uint32_t rx_tail = 0;  /* Read index */

/* UART handle */
static UART_HandleTypeDef *uart_handle = NULL;

/* Single byte buffer for interrupt reception */
static uint8_t rx_byte;

/* Optional callback for received characters */
static uart_in_rx_callback_t rx_callback = NULL;

/**
 * @brief Check if buffer is full
 */
static inline bool is_buffer_full(void)
{
	return ((rx_head + 1) & (UART_IN_BUFFER_SIZE - 1)) == rx_tail;
}

/**
 * @brief Check if buffer is empty
 */
static inline bool is_buffer_empty(void)
{
	return rx_head == rx_tail;
}

/**
 * @brief Add character to circular buffer
 */
static void buffer_put(uint8_t ch)
{
	uint32_t next_head = (rx_head + 1) & (UART_IN_BUFFER_SIZE - 1);

	if (next_head != rx_tail) {
		rx_buffer[rx_head] = ch;
		rx_head = next_head;
	} else {
		/* Buffer overflow - character is lost */
		printf("UART RX buffer overflow\n");
	}
}

/**
 * @brief Get character from circular buffer
 */
static bool buffer_get(uint8_t *ch)
{
	if (is_buffer_empty()) {
		return false;
	}

	*ch = rx_buffer[rx_tail];
	rx_tail = (rx_tail + 1) & (UART_IN_BUFFER_SIZE - 1);
	return true;
}

int uart_in_init(UART_HandleTypeDef *huart)
{
	if (!huart) {
		printf("uart_in_init: Invalid UART handle\n");
		return -1;
	}

	uart_handle = huart;

	/* Clear buffer */
	rx_head = 0;
	rx_tail = 0;

	/* Start reception in interrupt mode */
	if (HAL_UART_Receive_IT(uart_handle, &rx_byte, 1) != HAL_OK) {
		printf("uart_in_init: Failed to start UART reception\n");
		return -1;
	}

	printf("UART input driver initialized\n");
	return 0;
}

void uart_in_set_callback(uart_in_rx_callback_t callback)
{
	rx_callback = callback;
}

uint32_t uart_in_available(void)
{
	return (rx_head - rx_tail) & (UART_IN_BUFFER_SIZE - 1);
}

bool uart_in_getchar(uint8_t *ch)
{
	return buffer_get(ch);
}

uint32_t uart_in_read(uint8_t *buf, uint32_t len)
{
	uint32_t count = 0;

	while (count < len && buffer_get(&buf[count])) {
		count++;
	}

	return count;
}

void uart_in_flush(void)
{
	rx_head = 0;
	rx_tail = 0;
}

void uart_in_irq_handler(UART_HandleTypeDef *huart)
{
	if (huart != uart_handle) {
		return;
	}

	/* Add received byte to buffer */
	buffer_put(rx_byte);

	/* Call user callback if registered */
	if (rx_callback) {
		rx_callback(rx_byte);
	}

	/* Restart reception for next byte */
	HAL_UART_Receive_IT(uart_handle, &rx_byte, 1);
}

/**
 * @brief HAL UART RX Complete Callback
 *
 * This is called by HAL_UART_IRQHandler when reception is complete.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	uart_in_irq_handler(huart);
}
