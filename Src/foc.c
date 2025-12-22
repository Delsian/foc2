/*
 * Copyright (c) 2025 FOC2 Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "foc.h"
#include "drv/pwm.h"
#include <stdio.h>
#include <string.h>

#define M_PI_F 3.14159265358979323846f

/* Motor instances */
static struct foc_motor foc_motor0 = {
	.name = "motor0",
	.pwm_dev = NULL,
	.velocity_cfg = {
		.mode = FOC_VELOCITY_DISABLED,
		.target_rpm = 0.0f,
		.update_rate_hz = 0.0f,
		.acceleration = 1000.0f,  /* Default 1000 RPM/s */
		.pole_pairs = 7,          /* Default 7 pole pairs */
	},
	.current_rpm = 0.0f,
	.electrical_angle = 0.0f,
	.amplitude = 0.0f,
};

static struct foc_motor foc_motor1 = {
	.name = "motor1",
	.pwm_dev = NULL,
	.velocity_cfg = {
		.mode = FOC_VELOCITY_DISABLED,
		.target_rpm = 0.0f,
		.update_rate_hz = 0.0f,
		.acceleration = 1000.0f,  /* Default 1000 RPM/s */
		.pole_pairs = 7,          /* Default 7 pole pairs */
	},
	.current_rpm = 0.0f,
	.electrical_angle = 0.0f,
	.amplitude = 0.0f,
};

int foc_velocity_enable(struct foc_motor *motor, enum foc_velocity_mode mode,
                       float target_rpm, float amplitude, float update_rate_hz,
                       uint8_t pole_pairs)
{
	if (!motor || !motor->pwm_dev) {
		printf("FOC: Motor not initialized\n");
		return -1;
	}

	if (mode == FOC_VELOCITY_DISABLED) {
		printf("%s: Invalid mode (use foc_velocity_disable)\n", motor->name);
		return -1;
	}

	if (update_rate_hz <= 0.0f) {
		printf("%s: Invalid update rate: %d Hz\n", motor->name, (int)update_rate_hz);
		return -1;
	}

	if (pole_pairs == 0) {
		printf("%s: Invalid pole pairs: %u\n", motor->name, pole_pairs);
		return -1;
	}

	/* Configure velocity control */
	motor->velocity_cfg.mode = mode;
	motor->velocity_cfg.target_rpm = target_rpm;
	motor->velocity_cfg.update_rate_hz = update_rate_hz;
	motor->velocity_cfg.pole_pairs = pole_pairs;
	motor->velocity_cfg.acceleration = 1000.0f;
	motor->current_rpm = 0.0f;
	motor->electrical_angle = 0.0f;
	motor->amplitude = amplitude;

	printf("%s: Velocity control enabled - mode=%d, target=%d RPM, rate=%d Hz, poles=%u\n",
		motor->name, mode, (int)target_rpm, (int)update_rate_hz, pole_pairs);

	return 0;
}

int foc_velocity_disable(struct foc_motor *motor)
{
	if (!motor || !motor->pwm_dev) {
		printf("FOC: Motor not initialized\n");
		return -1;
	}

	motor->velocity_cfg.mode = FOC_VELOCITY_DISABLED;
	motor->current_rpm = 0.0f;
	motor->electrical_angle = 0.0f;

	printf("%s: Velocity control disabled\n", motor->name);
	return 0;
}

int foc_velocity_set_target(struct foc_motor *motor, float target_rpm)
{
	if (!motor || !motor->pwm_dev) {
		printf("FOC: Motor not initialized\n");
		return -1;
	}

	if (motor->velocity_cfg.mode == FOC_VELOCITY_DISABLED) {
		printf("%s: Velocity control not enabled\n", motor->name);
		return -1;
	}

	motor->velocity_cfg.target_rpm = target_rpm;
	return 0;
}

int foc_velocity_get_current(struct foc_motor *motor, float *rpm)
{
	if (!motor || !rpm) {
		return -1;
	}

	*rpm = motor->current_rpm;
	return 0;
}

void foc_velocity_update(struct foc_motor *motor)
{
	struct foc_velocity_config *cfg = &motor->velocity_cfg;
	float rpm_step, mechanical_rpm, electrical_rpm;
	float angle_step_deg;

	if (!motor || !motor->pwm_dev || cfg->mode == FOC_VELOCITY_DISABLED) {
		return;
	}

	/* Calculate RPM acceleration step per update */
	rpm_step = cfg->acceleration / cfg->update_rate_hz;

	/* Ramp current RPM towards target with acceleration limit */
	if (motor->current_rpm < cfg->target_rpm) {
		motor->current_rpm += rpm_step;
		if (motor->current_rpm > cfg->target_rpm) {
			motor->current_rpm = cfg->target_rpm;
		}
	} else if (motor->current_rpm > cfg->target_rpm) {
		motor->current_rpm -= rpm_step;
		if (motor->current_rpm < cfg->target_rpm) {
			motor->current_rpm = cfg->target_rpm;
		}
	}

	/* Convert mechanical RPM to electrical RPM
	 * Electrical RPM = Mechanical RPM × pole_pairs
	 */
	mechanical_rpm = motor->current_rpm;
	electrical_rpm = mechanical_rpm * (float)cfg->pole_pairs;

	/* Calculate angle increment per update period
	 * angle_step = (electrical_rpm / 60) * (360 / update_rate_hz)
	 *            = electrical_rpm * 6 / update_rate_hz
	 */
	angle_step_deg = (electrical_rpm * 360.0f) / (60.0f * cfg->update_rate_hz);

	/* Update electrical angle */
	motor->electrical_angle += angle_step_deg;

	/* Normalize angle to 0-360 degrees */
	while (motor->electrical_angle >= 360.0f) {
		motor->electrical_angle -= 360.0f;
	}
	while (motor->electrical_angle < 0.0f) {
		motor->electrical_angle += 360.0f;
	}

	/* Update PWM vector with current angle and amplitude */
	pwm_set_vector(motor->pwm_dev, motor->electrical_angle, motor->amplitude);
}

struct foc_motor *foc_get_motor(const char *name)
{
	if (strcmp(name, "motor0") == 0) {
		/* Lazy initialization of PWM device */
		if (!foc_motor0.pwm_dev) {
			foc_motor0.pwm_dev = pwm_get_device("pwm_motor0");
		}
		return &foc_motor0;
	} else if (strcmp(name, "motor1") == 0) {
		/* Lazy initialization of PWM device */
		if (!foc_motor1.pwm_dev) {
			foc_motor1.pwm_dev = pwm_get_device("pwm_motor1");
		}
		return &foc_motor1;
	}

	printf("Unknown motor: %s\n", name);
	return NULL;
}

void foc_task(void)
{
	/* Update velocity control for all motors */
	foc_velocity_update(&foc_motor0);
	foc_velocity_update(&foc_motor1);
}
