/*
 * Copyright (c) 2025 FOC2 Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADC_DMA_H
#define ADC_DMA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief ADC DMA driver for periodic analog channel sampling
 *
 * This driver uses DMA to periodically read ADC channels triggered by
 * a timer at PWM frequency and stores results in a buffer.
 */

#define ADC_DMA_NUM_CHANNELS  5    /* Number of ADC channels to read */
#define ADC_VREF_MV           3300 /* ADC reference voltage in millivolts */

/**
 * @brief Callback function type for ADC conversion complete
 *
 * @param values Array of ADC values (one per channel)
 * @param num_channels Number of channels in the array
 */
typedef void (*adc_dma_callback_t)(uint16_t *values, uint8_t num_channels);

/**
 * @brief Initialize ADC DMA driver
 *
 * Configures ADC2 to read all 5 channels using DMA, triggered by TIM2
 * at PWM frequency (20 kHz).
 *
 * @param hadc Pointer to ADC handle (e.g., &hadc2)
 * @param hdma Pointer to DMA handle (e.g., &hdma_adc2)
 * @param htim Pointer to timer handle for trigger (e.g., &htim2)
 * @return 0 on success, negative value on failure
 */
int adc_dma_init(ADC_HandleTypeDef *hadc, DMA_HandleTypeDef *hdma, TIM_HandleTypeDef *htim);

/**
 * @brief Start ADC DMA conversions
 *
 * @return 0 on success, negative value on failure
 */
int adc_dma_start(void);

/**
 * @brief Stop ADC DMA conversions
 *
 * @return 0 on success, negative value on failure
 */
int adc_dma_stop(void);

/**
 * @brief Get latest ADC value for a channel
 *
 * Returns the most recently sampled ADC value for the specified channel.
 * This is a non-blocking read from the DMA buffer.
 *
 * @param channel Channel index (0-4 for channels 1,2,3,4,12)
 * @param value Pointer to store the 12-bit ADC value (0-4095)
 * @return 0 on success, negative value on failure
 */
int adc_dma_get_channel(uint8_t channel, uint16_t *value);

/**
 * @brief Get all ADC channel values
 *
 * Copies all current ADC channel values to the provided buffer.
 * This is a non-blocking read from the DMA buffer.
 *
 * @param values Buffer to store ADC values (must hold ADC_DMA_NUM_CHANNELS values)
 * @param num_channels Number of channels to read (max ADC_DMA_NUM_CHANNELS)
 * @return 0 on success, negative value on failure
 */
int adc_dma_get_all_channels(uint16_t *values, uint8_t num_channels);

/**
 * @brief Convert ADC raw value to millivolts
 *
 * Converts a 12-bit ADC value to millivolts based on the reference voltage.
 *
 * @param raw_value Raw ADC value (0-4095)
 * @return Voltage in millivolts
 */
uint32_t adc_dma_raw_to_mv(uint16_t raw_value);

/**
 * @brief Register callback for conversion complete events
 *
 * @param callback Callback function to call when conversion completes
 */
void adc_dma_set_callback(adc_dma_callback_t callback);

/**
 * @brief ADC DMA conversion complete callback
 *
 * This should be called from HAL_ADC_ConvCpltCallback.
 *
 * @param hadc Pointer to ADC handle
 */
void adc_dma_conv_cplt_callback(ADC_HandleTypeDef *hadc);

#ifdef __cplusplus
}
#endif

#endif /* ADC_DMA_H */
