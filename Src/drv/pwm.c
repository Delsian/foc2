/*
 * Copyright (c) 2025 FOC2 Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "drv/pwm.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define M_PI_F 3.14159265358979323846f

/* External timer handles (declared in main.c) */
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;

/* Device configurations */
static const struct pwm_config pwm_motor0_config = {
	.htim = &htim2,
	.channel_a = TIM_CHANNEL_1,
	.channel_b = TIM_CHANNEL_2,
	.channel_c = TIM_CHANNEL_3,
	.pwm_frequency_hz = 20000,  /* 20 kHz */
};

static const struct pwm_config pwm_motor1_config = {
	.htim = &htim3,
	.channel_a = TIM_CHANNEL_2,
	.channel_b = TIM_CHANNEL_3,
	.channel_c = TIM_CHANNEL_4,
	.pwm_frequency_hz = 20000,  /* 20 kHz */
};

/* Device runtime data */
static struct pwm_data pwm_motor0_data = {
	.initialized = false,
	.phase = 0.0f,
	.duty = 0.0f,
};

static struct pwm_data pwm_motor1_data = {
	.initialized = false,
	.phase = 0.0f,
	.duty = 0.0f,
};

/* Device instances */
static struct pwm_device pwm_device_motor0 = {
	.name = "pwm_motor0",
	.config = &pwm_motor0_config,
	.data = &pwm_motor0_data,
};

static struct pwm_device pwm_device_motor1 = {
	.name = "pwm_motor1",
	.config = &pwm_motor1_config,
	.data = &pwm_motor1_data,
};

/**
 * @brief Clamp float value to range
 */
static inline float clamp_float(float value, float min, float max)
{
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

int pwm_init(struct pwm_device *dev)
{
	const struct pwm_config *config = dev->config;
	struct pwm_data *data = dev->data;
	TIM_OC_InitTypeDef sConfigOC = {0};

	if (!config->htim) {
		printf("%s: Timer handle is NULL\n", dev->name);
		return -1;
	}

	/* Configure timer for PWM at 20kHz (if not TIM2, which is used for ADC trigger) */
	/* TIM2 is already configured by adc_dma_init, so we skip base init for TIM2 */
	if (config->htim->Instance != TIM2) {
		config->htim->Init.Prescaler = 0;
		config->htim->Init.CounterMode = TIM_COUNTERMODE_UP;
		config->htim->Init.Period = 8499;  /* 20 kHz at 170 MHz */
		config->htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
		config->htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

		if (HAL_TIM_PWM_Init(config->htim) != HAL_OK) {
			printf("%s: Timer PWM init failed\n", dev->name);
			return -1;
		}
	}

	/* Configure PWM channels */
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = 0;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

	if (HAL_TIM_PWM_ConfigChannel(config->htim, &sConfigOC, config->channel_a) != HAL_OK) {
		printf("%s: Failed to configure PWM channel A\n", dev->name);
		return -1;
	}

	if (HAL_TIM_PWM_ConfigChannel(config->htim, &sConfigOC, config->channel_b) != HAL_OK) {
		printf("%s: Failed to configure PWM channel B\n", dev->name);
		return -1;
	}

	if (HAL_TIM_PWM_ConfigChannel(config->htim, &sConfigOC, config->channel_c) != HAL_OK) {
		printf("%s: Failed to configure PWM channel C\n", dev->name);
		return -1;
	}

	/* Get timer period (ARR value) */
	uint32_t period = __HAL_TIM_GET_AUTORELOAD(config->htim);

	/* Initialize all channels to 0% duty cycle */
	__HAL_TIM_SET_COMPARE(config->htim, config->channel_a, 0);
	__HAL_TIM_SET_COMPARE(config->htim, config->channel_b, 0);
	__HAL_TIM_SET_COMPARE(config->htim, config->channel_c, 0);

	data->initialized = true;
	printf("%s: PWM initialized (period: %lu, freq: %lu Hz)\n",
		dev->name, period, config->pwm_frequency_hz);

	return 0;
}

int pwm_start(struct pwm_device *dev)
{
	const struct pwm_config *config = dev->config;
	struct pwm_data *data = dev->data;

	if (!data->initialized) {
		printf("%s: Device not initialized\n", dev->name);
		return -1;
	}

	/* Start PWM generation on all three channels */
	if (HAL_TIM_PWM_Start(config->htim, config->channel_a) != HAL_OK) {
		printf("%s: Failed to start PWM channel A\n", dev->name);
		return -1;
	}

	if (HAL_TIM_PWM_Start(config->htim, config->channel_b) != HAL_OK) {
		printf("%s: Failed to start PWM channel B\n", dev->name);
		return -1;
	}

	if (HAL_TIM_PWM_Start(config->htim, config->channel_c) != HAL_OK) {
		printf("%s: Failed to start PWM channel C\n", dev->name);
		return -1;
	}

	printf("%s: PWM started\n", dev->name);
	return 0;
}

int pwm_stop(struct pwm_device *dev)
{
	const struct pwm_config *config = dev->config;
	struct pwm_data *data = dev->data;

	if (!data->initialized) {
		printf("%s: Device not initialized\n", dev->name);
		return -1;
	}

	/* Stop PWM generation on all three channels */
	HAL_TIM_PWM_Stop(config->htim, config->channel_a);
	HAL_TIM_PWM_Stop(config->htim, config->channel_b);
	HAL_TIM_PWM_Stop(config->htim, config->channel_c);

	printf("%s: PWM stopped\n", dev->name);
	return 0;
}

int pwm_set_duty(struct pwm_device *dev, float duty_a, float duty_b, float duty_c)
{
	const struct pwm_config *config = dev->config;
	struct pwm_data *data = dev->data;

	if (!data->initialized) {
		printf("%s: Device not initialized\n", dev->name);
		return -1;
	}

	/* Clamp duty cycles to 0-100% */
	duty_a = clamp_float(duty_a, 0.0f, 100.0f);
	duty_b = clamp_float(duty_b, 0.0f, 100.0f);
	duty_c = clamp_float(duty_c, 0.0f, 100.0f);

	/* Get timer period */
	uint32_t period = __HAL_TIM_GET_AUTORELOAD(config->htim);

	/* Convert duty cycle percentage to compare value */
	uint32_t compare_a = (uint32_t)((duty_a / 100.0f) * period);
	uint32_t compare_b = (uint32_t)((duty_b / 100.0f) * period);
	uint32_t compare_c = (uint32_t)((duty_c / 100.0f) * period);

	/* Set PWM duty cycles */
	__HAL_TIM_SET_COMPARE(config->htim, config->channel_a, compare_a);
	__HAL_TIM_SET_COMPARE(config->htim, config->channel_b, compare_b);
	__HAL_TIM_SET_COMPARE(config->htim, config->channel_c, compare_c);

	return 0;
}

int pwm_set_phase_duty(struct pwm_device *dev, uint8_t phase, float duty)
{
	const struct pwm_config *config = dev->config;
	struct pwm_data *data = dev->data;

	if (!data->initialized) {
		printf("%s: Device not initialized\n", dev->name);
		return -1;
	}

	if (phase > 2) {
		printf("%s: Invalid phase %u (must be 0-2)\n", dev->name, phase);
		return -1;
	}

	/* Clamp duty cycle to 0-100% */
	duty = clamp_float(duty, 0.0f, 100.0f);

	/* Get timer period */
	uint32_t period = __HAL_TIM_GET_AUTORELOAD(config->htim);

	/* Convert duty cycle percentage to compare value */
	uint32_t compare = (uint32_t)((duty / 100.0f) * period);

	/* Set the appropriate PWM channel */
	switch (phase) {
	case 0:  /* Phase A */
		__HAL_TIM_SET_COMPARE(config->htim, config->channel_a, compare);
		break;
	case 1:  /* Phase B */
		__HAL_TIM_SET_COMPARE(config->htim, config->channel_b, compare);
		break;
	case 2:  /* Phase C */
		__HAL_TIM_SET_COMPARE(config->htim, config->channel_c, compare);
		break;
	default:
		return -1;
	}

	return 0;
}

int pwm_set_vector(struct pwm_device *dev, float angle_deg, float amplitude)
{
	const struct pwm_config *config = dev->config;
	struct pwm_data *data = dev->data;
	float angle_rad;
	float duty_a, duty_b, duty_c;
	float sine_a, sine_b, sine_c;

	if (!data->initialized) {
		printf("%s: Device not initialized\n", dev->name);
		return -1;
	}

	/* Clamp amplitude to 0-100% */
	amplitude = clamp_float(amplitude, 0.0f, 100.0f);

	/* Normalize angle to 0-360 degrees */
	while (angle_deg < 0.0f) angle_deg += 360.0f;
	while (angle_deg >= 360.0f) angle_deg -= 360.0f;

	/* Convert to radians */
	angle_rad = angle_deg * M_PI_F / 180.0f;

	/* Calculate three-phase sinusoidal values with 120-degree spacing
	 * Phase A: sin(θ)
	 * Phase B: sin(θ - 120°)
	 * Phase C: sin(θ + 120°)
	 */
	sine_a = sinf(angle_rad);
	sine_b = sinf(angle_rad - 2.0f * M_PI_F / 3.0f);  /* -120 degrees */
	sine_c = sinf(angle_rad + 2.0f * M_PI_F / 3.0f);  /* +120 degrees */

	/* Convert sine values (-1 to +1) to duty cycles (0 to 100%)
	 * Using bipolar modulation: duty = 50% + (sine * amplitude/2)
	 */
	duty_a = 50.0f + (sine_a * amplitude / 2.0f);
	duty_b = 50.0f + (sine_b * amplitude / 2.0f);
	duty_c = 50.0f + (sine_c * amplitude / 2.0f);

	/* Clamp to valid range */
	duty_a = clamp_float(duty_a, 0.0f, 100.0f);
	duty_b = clamp_float(duty_b, 0.0f, 100.0f);
	duty_c = clamp_float(duty_c, 0.0f, 100.0f);

	/* Get timer period */
	uint32_t period = __HAL_TIM_GET_AUTORELOAD(config->htim);

	/* Convert duty cycles to compare values */
	uint32_t compare_a = (uint32_t)((duty_a / 100.0f) * period);
	uint32_t compare_b = (uint32_t)((duty_b / 100.0f) * period);
	uint32_t compare_c = (uint32_t)((duty_c / 100.0f) * period);

	/* Set PWM outputs */
	__HAL_TIM_SET_COMPARE(config->htim, config->channel_a, compare_a);
	__HAL_TIM_SET_COMPARE(config->htim, config->channel_b, compare_b);
	__HAL_TIM_SET_COMPARE(config->htim, config->channel_c, compare_c);

	/* Store current values */
	data->phase = angle_deg;
	data->duty = amplitude;

	return 0;
}

int pwm_disable(struct pwm_device *dev)
{
	const struct pwm_config *config = dev->config;
	struct pwm_data *data = dev->data;

	if (!data->initialized) {
		printf("%s: Device not initialized\n", dev->name);
		return -1;
	}

	/* Set all channels to 0% duty cycle */
	__HAL_TIM_SET_COMPARE(config->htim, config->channel_a, 0);
	__HAL_TIM_SET_COMPARE(config->htim, config->channel_b, 0);
	__HAL_TIM_SET_COMPARE(config->htim, config->channel_c, 0);

	printf("%s: PWM outputs disabled\n", dev->name);
	return 0;
}

struct pwm_device *pwm_get_device(const char *name)
{
	if (strcmp(name, "pwm_motor0") == 0) {
		return &pwm_device_motor0;
	} else if (strcmp(name, "pwm_motor1") == 0) {
		return &pwm_device_motor1;
	}

	printf("Unknown PWM device: %s\n", name);
	return NULL;
}
