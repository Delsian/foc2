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
 * @brief Velocity control mode
 */
enum pwm_velocity_mode {
	PWM_VELOCITY_DISABLED = 0,   /* Velocity control disabled */
	PWM_VELOCITY_OPEN_LOOP,      /* Open-loop velocity control */
	PWM_VELOCITY_CLOSED_LOOP,    /* Closed-loop with encoder feedback */
};

/**
 * @brief Velocity control configuration
 */
struct pwm_velocity_config {
	enum pwm_velocity_mode mode; /* Control mode */
	float target_rpm;            /* Target rotational speed in RPM */
	float update_rate_hz;        /* Update rate in Hz */
	float acceleration;          /* Acceleration limit in RPM/s */
	uint8_t pole_pairs;          /* Number of motor pole pairs */
};

/**
 * @brief PWM device runtime data
 */
struct pwm_data {
	bool initialized;
	float phase;                 /* Current phase angle in degrees */
	float duty;                  /* Current duty cycle in percentage */

	/* Velocity control */
	struct pwm_velocity_config velocity_cfg;
	float current_rpm;           /* Current velocity in RPM */
	float electrical_angle;      /* Current electrical angle in degrees */
	uint32_t update_counter;     /* Update counter for timing */
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

/**
 * @brief Enable velocity control mode
 *
 * @param dev Pointer to PWM device
 * @param mode Velocity control mode
 * @param target_rpm Target velocity in RPM
 * @param amplitude PWM amplitude/magnitude (0-100%)
 * @param update_rate_hz Control loop update rate in Hz
 * @param pole_pairs Number of motor pole pairs
 * @return 0 on success, negative value on failure
 */
int pwm_velocity_enable(struct pwm_device *dev, enum pwm_velocity_mode mode,
                       float target_rpm, float amplitude, float update_rate_hz,
                       uint8_t pole_pairs);

/**
 * @brief Disable velocity control mode
 *
 * @param dev Pointer to PWM device
 * @return 0 on success, negative value on failure
 */
int pwm_velocity_disable(struct pwm_device *dev);

/**
 * @brief Set target velocity
 *
 * @param dev Pointer to PWM device
 * @param target_rpm Target velocity in RPM
 * @return 0 on success, negative value on failure
 */
int pwm_velocity_set_target(struct pwm_device *dev, float target_rpm);

/**
 * @brief Get current velocity
 *
 * @param dev Pointer to PWM device
 * @param rpm Pointer to store current velocity in RPM
 * @return 0 on success, negative value on failure
 */
int pwm_velocity_get_current(struct pwm_device *dev, float *rpm);

/**
 * @brief Periodical PWM task, sync with timer
 *
 * This function handles velocity control updates and should be called
 * from a timer interrupt or periodically from the main loop.
 */
void pwm_task(void);

#ifdef __cplusplus
}
#endif

#endif /* PWM_H */
