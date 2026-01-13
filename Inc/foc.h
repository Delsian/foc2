/*
 * Copyright (c) 2025 FOC2 Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FOC_H
#define FOC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "drv/pwm.h"
#include <stdbool.h>

/**
 * @brief FOC motor control
 *
 * This module provides high-level motor control functions including
 * velocity control for BLDC motors with optional encoder feedback
 * for closed-loop control.
 */

#include "drv/mt6701.h"

/**
 * @brief Velocity control mode
 */
enum foc_velocity_mode {
	FOC_VELOCITY_DISABLED = 0,   /* Velocity control disabled */
	FOC_VELOCITY_OPEN_LOOP,      /* Open-loop velocity control */
	FOC_VELOCITY_CLOSED_LOOP,    /* Closed-loop with encoder feedback */
};

/**
 * @brief Velocity control configuration
 */
struct foc_velocity_config {
	enum foc_velocity_mode mode; /* Control mode */
	float target_rpm;            /* Target rotational speed in RPM */
	float update_rate_hz;        /* Update rate in Hz */
	float acceleration;          /* Acceleration limit in RPM/s */
	uint8_t pole_pairs;          /* Number of motor pole pairs */
};

/**
 * @brief Current sensing configuration
 */
struct foc_current_config {
	uint8_t adc_channel_a;       /* ADC channel for phase A current */
	uint8_t adc_channel_b;       /* ADC channel for phase B current */
	float current_sensitivity;   /* Current sensor sensitivity (V/A) */
	float current_offset;        /* Current sensor offset (V) */
	float current_limit_a;       /* Maximum allowed current (A) */
	bool enabled;                /* Current sensing enabled */
};

/**
 * @brief Current sensing data
 */
struct foc_current_data {
	float phase_a_current;       /* Phase A current in Amps */
	float phase_b_current;       /* Phase B current in Amps */
	float phase_c_current;       /* Phase C current (calculated) in Amps */
	float magnitude;             /* Current magnitude in Amps */
	bool overcurrent;            /* Overcurrent flag */
};

/**
 * @brief Encoder configuration
 */
struct foc_encoder_config {
	mt6701_t *encoder;           /* Pointer to encoder device */
	bool enabled;                /* Encoder feedback enabled */
	float mechanical_offset;     /* Mechanical offset in degrees (0-360) */
	uint8_t pole_pairs;          /* Motor pole pairs for electrical angle calculation */
	bool invert_direction;       /* Invert rotation direction */
};

/**
 * @brief Encoder runtime data
 */
struct foc_encoder_data {
	float mechanical_angle;      /* Current mechanical angle in degrees (0-360) */
	float electrical_angle;      /* Current electrical angle in degrees (0-360) */
	float velocity_rpm;          /* Measured velocity in RPM */
	uint32_t last_update_ms;     /* Last update timestamp */
	float last_angle;            /* Last angle for velocity calculation */
	float filtered_velocity;     /* Filtered velocity for smoothing */
};

/**
 * @brief FOC motor instance
 */
struct foc_motor {
	const char *name;
	struct pwm_device *pwm_dev;

	/* Velocity control */
	struct foc_velocity_config velocity_cfg;
	float current_rpm;           /* Current velocity in RPM */
	float electrical_angle;      /* Current electrical angle in degrees */
	float amplitude;             /* PWM amplitude/magnitude (0-100%) */

	/* Current sensing */
	struct foc_current_config current_cfg;
	struct foc_current_data current_data;

	/* Encoder feedback */
	struct foc_encoder_config encoder_cfg;
	struct foc_encoder_data encoder_data;
};

/**
 * @brief Enable velocity control mode
 *
 * @param motor Pointer to FOC motor instance
 * @param mode Velocity control mode
 * @param target_rpm Target velocity in RPM
 * @param amplitude PWM amplitude/magnitude (0-100%)
 * @param update_rate_hz Control loop update rate in Hz
 * @param pole_pairs Number of motor pole pairs
 * @return 0 on success, negative value on failure
 */
int foc_velocity_enable(struct foc_motor *motor, enum foc_velocity_mode mode,
                       float target_rpm, float amplitude, float update_rate_hz,
                       uint8_t pole_pairs);

/**
 * @brief Disable velocity control mode
 *
 * @param motor Pointer to FOC motor instance
 * @return 0 on success, negative value on failure
 */
int foc_velocity_disable(struct foc_motor *motor);

/**
 * @brief Set target velocity
 *
 * @param motor Pointer to FOC motor instance
 * @param target_rpm Target velocity in RPM
 * @return 0 on success, negative value on failure
 */
int foc_velocity_set_target(struct foc_motor *motor, float target_rpm);

/**
 * @brief Get current velocity
 *
 * @param motor Pointer to FOC motor instance
 * @param rpm Pointer to store current velocity in RPM
 * @return 0 on success, negative value on failure
 */
int foc_velocity_get_current(struct foc_motor *motor, float *rpm);

/**
 * @brief Update velocity control for a motor
 *
 * This function should be called periodically from the control loop
 *
 * @param motor Pointer to FOC motor instance
 */
void foc_velocity_update(struct foc_motor *motor);

/**
 * @brief Get FOC motor instance by name
 *
 * @param name Motor name (e.g., "motor0", "motor1")
 * @return Pointer to motor instance or NULL if not found
 */
struct foc_motor *foc_get_motor(const char *name);

/**
 * @brief Periodical FOC task for all motors
 *
 * This function updates velocity control for all motors and should be
 * called from a timer interrupt or periodically from the main loop.
 */
void foc_task(void);

/**
 * @brief Configure current sensing for a motor
 *
 * @param motor Pointer to FOC motor instance
 * @param adc_ch_a ADC channel for phase A current (0-4)
 * @param adc_ch_b ADC channel for phase B current (0-4)
 * @param sensitivity Current sensor sensitivity in V/A (e.g., 0.2 for INA181A1 with 0.01Ω shunt)
 * @param offset Current sensor offset voltage in V (0V for unidirectional INA181A1)
 * @param limit_a Maximum allowed current in Amps
 * @return 0 on success, negative value on failure
 */
int foc_current_config(struct foc_motor *motor, uint8_t adc_ch_a, uint8_t adc_ch_b,
                       float sensitivity, float offset, float limit_a);

/**
 * @brief Enable current sensing
 *
 * @param motor Pointer to FOC motor instance
 * @return 0 on success, negative value on failure
 */
int foc_current_enable(struct foc_motor *motor);

/**
 * @brief Disable current sensing
 *
 * @param motor Pointer to FOC motor instance
 * @return 0 on success, negative value on failure
 */
int foc_current_disable(struct foc_motor *motor);

/**
 * @brief Update current measurements for a motor
 *
 * Reads ADC values, converts to currents, and checks limits.
 * Should be called periodically (e.g., from foc_task).
 *
 * @param motor Pointer to FOC motor instance
 */
void foc_current_update(struct foc_motor *motor);

/**
 * @brief Get current motor current
 *
 * @param motor Pointer to FOC motor instance
 * @param current_a Pointer to store current magnitude in Amps (can be NULL)
 * @return 0 on success, negative value on failure
 */
int foc_current_get(struct foc_motor *motor, float *current_a);

/**
 * @brief Check if motor is in overcurrent condition
 *
 * @param motor Pointer to FOC motor instance
 * @return true if overcurrent detected, false otherwise
 */
bool foc_current_is_overcurrent(struct foc_motor *motor);

/**
 * @brief Set current limit for overcurrent protection
 *
 * @param motor Pointer to FOC motor instance
 * @param limit_a Current limit in Amps
 * @return 0 on success, negative value on failure
 */
int foc_current_set_limit(struct foc_motor *motor, float limit_a);

/**
 * @brief Configure encoder for closed-loop control
 *
 * @param motor Pointer to FOC motor instance
 * @param encoder Pointer to MT6701 encoder device
 * @param pole_pairs Number of motor pole pairs
 * @param mechanical_offset Mechanical offset in degrees (0-360)
 * @param invert_direction Set to true to invert rotation direction
 * @return 0 on success, negative value on failure
 */
int foc_encoder_config(struct foc_motor *motor, mt6701_t *encoder,
                       uint8_t pole_pairs, float mechanical_offset,
                       bool invert_direction);

/**
 * @brief Enable encoder feedback for closed-loop control
 *
 * When enabled, the motor will use encoder feedback for:
 * - Accurate rotor position (commutation angle)
 * - Real-time velocity measurement
 * - Closed-loop velocity control
 *
 * @param motor Pointer to FOC motor instance
 * @return 0 on success, negative value on failure
 */
int foc_encoder_enable(struct foc_motor *motor);

/**
 * @brief Disable encoder feedback (revert to open-loop)
 *
 * @param motor Pointer to FOC motor instance
 * @return 0 on success, negative value on failure
 */
int foc_encoder_disable(struct foc_motor *motor);

/**
 * @brief Update encoder readings and calculate electrical angle
 *
 * This function should be called periodically (e.g., from foc_task).
 * It reads the encoder, calculates electrical angle, and estimates velocity.
 *
 * @param motor Pointer to FOC motor instance
 */
void foc_encoder_update(struct foc_motor *motor);

/**
 * @brief Get current encoder angle and velocity
 *
 * @param motor Pointer to FOC motor instance
 * @param mechanical_angle Pointer to store mechanical angle in degrees (can be NULL)
 * @param electrical_angle Pointer to store electrical angle in degrees (can be NULL)
 * @param velocity_rpm Pointer to store velocity in RPM (can be NULL)
 * @return 0 on success, negative value on failure
 */
int foc_encoder_get(struct foc_motor *motor, float *mechanical_angle,
                    float *electrical_angle, float *velocity_rpm);

#ifdef __cplusplus
}
#endif

#endif /* FOC_H */
