/*
 * Copyright (c) 2025 FOC2 Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PWM_H
#define PWM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdbool.h>

/**
 * @brief PWM driver for BLDC motor control
 *
 * This driver provides 3-phase PWM control for BLDC motors
 * using STM32 timer peripherals.
 */

/**
 * @brief PWM device configuration
 */
struct pwm_config {
	TIM_HandleTypeDef *htim;     /* Timer handle */
	uint32_t channel_a;          /* Phase A channel (TIM_CHANNEL_x) */
	uint32_t channel_b;          /* Phase B channel (TIM_CHANNEL_x) */
	uint32_t channel_c;          /* Phase C channel (TIM_CHANNEL_x) */
	uint32_t pwm_frequency_hz;   /* PWM frequency in Hz */
};

/**
 * @brief PWM device runtime data
 */
struct pwm_data {
	bool initialized;
	float phase;                 /* Current phase angle in degrees */
	float duty;                  /* Current duty cycle in percentage */
};

/**
 * @brief PWM device instance
 */
struct pwm_device {
	const char *name;
	const struct pwm_config *config;
	struct pwm_data *data;
};

/**
 * @brief Initialize PWM device
 *
 * @param dev Pointer to PWM device
 * @return 0 on success, negative value on failure
 */
int pwm_init(struct pwm_device *dev);

/**
 * @brief Set duty cycle for all three phases with independent control
 *
 * @param dev Pointer to PWM device
 * @param duty_a Duty cycle for phase A (0-100%)
 * @param duty_b Duty cycle for phase B (0-100%)
 * @param duty_c Duty cycle for phase C (0-100%)
 * @return 0 on success, negative value on failure
 */
int pwm_set_duty(struct pwm_device *dev, float duty_a, float duty_b, float duty_c);

/**
 * @brief Set duty cycle for a single phase
 *
 * @param dev Pointer to PWM device
 * @param phase Phase number (0=A, 1=B, 2=C)
 * @param duty Duty cycle (0-100%)
 * @return 0 on success, negative value on failure
 */
int pwm_set_phase_duty(struct pwm_device *dev, uint8_t phase, float duty);

/**
 * @brief Set three-phase sinusoidal PWM with 120-degree spacing
 *
 * This generates a rotating magnetic field for smooth BLDC motor control.
 * Uses bipolar modulation centered at 50% duty cycle.
 *
 * @param dev Pointer to PWM device
 * @param angle_deg Electrical angle in degrees (0-360)
 * @param amplitude Amplitude/magnitude (0-100%)
 * @return 0 on success, negative value on failure
 */
int pwm_set_vector(struct pwm_device *dev, float angle_deg, float amplitude);

/**
 * @brief Set three-phase Space Vector PWM (SVPWM)
 *
 * Space Vector Modulation offers superior performance compared to sinusoidal PWM:
 * - ~15% higher voltage utilization (uses full DC bus voltage)
 * - Lower harmonic distortion and reduced torque ripple
 * - Better power efficiency and reduced switching losses
 * - Optimized for BLDC/PMSM motor control
 *
 * This implementation is based on the SimpleFOC library algorithm.
 *
 * @param dev Pointer to PWM device
 * @param angle_deg Electrical angle in degrees (0-360)
 * @param amplitude Amplitude/magnitude (0-100%)
 * @return 0 on success, negative value on failure
 */
int pwm_set_vector_svpwm(struct pwm_device *dev, float angle_deg, float amplitude);

/**
 * @brief Disable all PWM outputs (set to 0%)
 *
 * @param dev Pointer to PWM device
 * @return 0 on success, negative value on failure
 */
int pwm_disable(struct pwm_device *dev);

/**
 * @brief Start PWM generation on all channels
 *
 * @param dev Pointer to PWM device
 * @return 0 on success, negative value on failure
 */
int pwm_start(struct pwm_device *dev);

/**
 * @brief Stop PWM generation on all channels
 *
 * @param dev Pointer to PWM device
 * @return 0 on success, negative value on failure
 */
int pwm_stop(struct pwm_device *dev);

/**
 * @brief Get PWM device by name
 *
 * @param name Device name (e.g., "pwm_motor0", "pwm_motor1")
 * @return Pointer to device or NULL if not found
 */
struct pwm_device *pwm_get_device(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* PWM_H */
