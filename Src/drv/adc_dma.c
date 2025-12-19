/*
 * Copyright (c) 2025 FOC2 Project
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ADC Timing Analysis (20 kHz trigger):
 * - ADC clock: 42.5 MHz (PCLK/4)
 * - Sampling time: 47.5 cycles
 * - Conversion time: 12.5 cycles (12-bit)
 * - Time per channel: 60 cycles × 23.5ns = 1.41 µs
 * - Time for 5 channels: 7.05 µs
 * - Hardware oversampling: 4x (automatic averaging)
 * - Total conversion time with oversampling: ~28 µs
 * - Time between 20kHz triggers: 50 µs
 * - Margin: 22 µs (44% headroom)
 */

#include "drv/adc_dma.h"
#include <stdio.h>
#include <string.h>

/* ADC channels: 1, 2, 3, 4, 12 (PA0, PA1, PA6, PA7, PB2) */
static const uint32_t adc_channels[ADC_DMA_NUM_CHANNELS] = {
	ADC_CHANNEL_1,   /* PA0 */
	ADC_CHANNEL_2,   /* PA1 */
	ADC_CHANNEL_3,   /* PA6 */
	ADC_CHANNEL_4,   /* PA7 */
	ADC_CHANNEL_12   /* PB2 */
};

/* DMA buffer for ADC values */
static uint16_t adc_buffer[ADC_DMA_NUM_CHANNELS] __attribute__((aligned(4)));

/* Device handles */
static ADC_HandleTypeDef *adc_handle = NULL;
static DMA_HandleTypeDef *dma_handle = NULL;
static TIM_HandleTypeDef *tim_handle = NULL;

/* Initialization flag */
static bool initialized = false;

/* Optional conversion complete callback */
static adc_dma_callback_t conv_cplt_callback = NULL;

int adc_dma_init(ADC_HandleTypeDef *hadc, DMA_HandleTypeDef *hdma, TIM_HandleTypeDef *htim)
{
	ADC_ChannelConfTypeDef sConfig = {0};

	if (!hadc || !hdma || !htim) {
		printf("adc_dma_init: Invalid handles\n");
		return -1;
	}

	adc_handle = hadc;
	dma_handle = hdma;
	tim_handle = htim;

	/* Reconfigure ADC for DMA with timer trigger */
	adc_handle->Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
	adc_handle->Init.Resolution = ADC_RESOLUTION_12B;
	adc_handle->Init.DataAlign = ADC_DATAALIGN_RIGHT;
	adc_handle->Init.GainCompensation = 0;
	adc_handle->Init.ScanConvMode = ADC_SCAN_ENABLE;  /* Enable scan mode for multiple channels */
	adc_handle->Init.EOCSelection = ADC_EOC_SEQ_CONV; /* End of sequence */
	adc_handle->Init.LowPowerAutoWait = DISABLE;
	adc_handle->Init.ContinuousConvMode = DISABLE;    /* Triggered by timer */
	adc_handle->Init.NbrOfConversion = ADC_DMA_NUM_CHANNELS;
	adc_handle->Init.DiscontinuousConvMode = DISABLE;
	adc_handle->Init.ExternalTrigConv = ADC_EXTERNALTRIG_T2_TRGO;  /* TIM2 TRGO */
	adc_handle->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
	adc_handle->Init.DMAContinuousRequests = ENABLE;
	adc_handle->Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;

	/* Enable hardware oversampling for noise reduction */
	adc_handle->Init.OversamplingMode = ENABLE;
	adc_handle->Init.Oversampling.Ratio = ADC_OVERSAMPLING_RATIO_4;  /* 4x oversampling */
	adc_handle->Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_2;  /* Divide by 4 (>>2) */
	adc_handle->Init.Oversampling.TriggeredMode = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;
	adc_handle->Init.Oversampling.OversamplingStopReset = ADC_REGOVERSAMPLING_CONTINUED_MODE;

	if (HAL_ADC_Init(adc_handle) != HAL_OK) {
		printf("adc_dma_init: ADC init failed\n");
		return -1;
	}

	/* Rank values for STM32 HAL */
	const uint32_t ranks[ADC_DMA_NUM_CHANNELS] = {
		ADC_REGULAR_RANK_1,
		ADC_REGULAR_RANK_2,
		ADC_REGULAR_RANK_3,
		ADC_REGULAR_RANK_4,
		ADC_REGULAR_RANK_5
	};

	/* Configure all channels */
	for (uint8_t i = 0; i < ADC_DMA_NUM_CHANNELS; i++) {
		sConfig.Channel = adc_channels[i];
		sConfig.Rank = ranks[i];
		sConfig.SamplingTime = ADC_SAMPLETIME_47CYCLES_5;  /* Slower sampling for accuracy */
		sConfig.SingleDiff = ADC_SINGLE_ENDED;
		sConfig.OffsetNumber = ADC_OFFSET_NONE;
		sConfig.Offset = 0;

		if (HAL_ADC_ConfigChannel(adc_handle, &sConfig) != HAL_OK) {
			printf("adc_dma_init: Channel %lu config failed\n", adc_channels[i]);
			return -1;
		}
	}

	/* Perform ADC calibration */
	if (HAL_ADCEx_Calibration_Start(adc_handle, ADC_SINGLE_ENDED) != HAL_OK) {
		printf("adc_dma_init: ADC calibration failed\n");
		return -1;
	}

	/* Configure timer for 20kHz update rate
	 * Assuming system clock is 170MHz, APB1 timer clock = 170MHz
	 * For 20kHz: Period = 170MHz / 20kHz = 8500 - 1 = 8499
	 */
	tim_handle->Init.Prescaler = 0;
	tim_handle->Init.CounterMode = TIM_COUNTERMODE_UP;
	tim_handle->Init.Period = 8499;  /* 20 kHz at 170 MHz clock */
	tim_handle->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	tim_handle->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

	if (HAL_TIM_Base_Init(tim_handle) != HAL_OK) {
		printf("adc_dma_init: Timer init failed\n");
		return -1;
	}

	/* Configure timer to generate TRGO on update event */
	TIM_MasterConfigTypeDef sMasterConfig = {0};
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
	sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

	if (HAL_TIMEx_MasterConfigSynchronization(tim_handle, &sMasterConfig) != HAL_OK) {
		printf("adc_dma_init: Timer master config failed\n");
		return -1;
	}

	/* Clear buffer */
	memset(adc_buffer, 0, sizeof(adc_buffer));

	initialized = true;
	printf("ADC DMA initialized: %d channels, trigger: TIM2@20kHz\n", ADC_DMA_NUM_CHANNELS);

	return 0;
}

int adc_dma_start(void)
{
	if (!initialized) {
		printf("adc_dma_start: Not initialized\n");
		return -1;
	}

	/* Start ADC DMA */
	if (HAL_ADC_Start_DMA(adc_handle, (uint32_t *)adc_buffer, ADC_DMA_NUM_CHANNELS) != HAL_OK) {
		printf("adc_dma_start: Failed to start ADC DMA\n");
		return -1;
	}

	/* Start the timer to trigger conversions */
	if (HAL_TIM_Base_Start(tim_handle) != HAL_OK) {
		printf("adc_dma_start: Failed to start timer\n");
		return -1;
	}

	printf("ADC DMA started\n");
	return 0;
}

int adc_dma_stop(void)
{
	if (!initialized) {
		printf("adc_dma_stop: Not initialized\n");
		return -1;
	}

	/* Stop timer */
	HAL_TIM_Base_Stop(tim_handle);

	/* Stop ADC DMA */
	HAL_ADC_Stop_DMA(adc_handle);

	printf("ADC DMA stopped\n");
	return 0;
}

int adc_dma_get_channel(uint8_t channel, uint16_t *value)
{
	if (!value || channel >= ADC_DMA_NUM_CHANNELS) {
		return -1;
	}

	if (!initialized) {
		return -1;
	}

	*value = adc_buffer[channel];
	return 0;
}

int adc_dma_get_all_channels(uint16_t *values, uint8_t num_channels)
{
	if (!values || num_channels > ADC_DMA_NUM_CHANNELS) {
		return -1;
	}

	if (!initialized) {
		return -1;
	}

	memcpy(values, adc_buffer, num_channels * sizeof(uint16_t));
	return 0;
}

uint32_t adc_dma_raw_to_mv(uint16_t raw_value)
{
	return ((uint32_t)raw_value * ADC_VREF_MV) / 4096;
}

void adc_dma_set_callback(adc_dma_callback_t callback)
{
	conv_cplt_callback = callback;
}

void adc_dma_conv_cplt_callback(ADC_HandleTypeDef *hadc)
{
	if (hadc != adc_handle) {
		return;
	}

	/* Call user callback if registered */
	if (conv_cplt_callback) {
		conv_cplt_callback(adc_buffer, ADC_DMA_NUM_CHANNELS);
	}
}

/**
 * @brief HAL ADC conversion complete callback
 *
 * This is called by HAL when DMA transfer completes.
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	adc_dma_conv_cplt_callback(hadc);
}
