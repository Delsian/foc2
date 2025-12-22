/*
 * Copyright (c) 2025 FOC2 Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "foc.h"
#include "drv/pwm.h"
#include "drv/adc_dma.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

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
	.current_cfg = {
		.enabled = false,         /* Disabled by default */
		.adc_channel_a = 0,
		.adc_channel_b = 1,
		.current_sensitivity = 1.2f,
		.current_offset = 0.0f,   /* Unidirectional sensor, no offset */
		.current_limit_a = 2.0f,  /* 2A default limit */
	},
	.current_data = {0},
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
	.current_cfg = {
		.enabled = false,         /* Disabled by default */
		.adc_channel_a = 2,
		.adc_channel_b = 3,
		.current_sensitivity = 1.2f,  /* 1200mV/A */
		.current_offset = 0.0f,   /* Unidirectional sensor, no offset */
		.current_limit_a = 2.0f,  /* 2A default limit */
	},
	.current_data = {0},
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

	/* Update current sensing for all motors */
	foc_current_update(&foc_motor0);
	foc_current_update(&foc_motor1);
}

int foc_current_config(struct foc_motor *motor, uint8_t adc_ch_a, uint8_t adc_ch_b,
                       float sensitivity, float offset, float limit_a)
{
	if (!motor) {
		printf("FOC: Motor not initialized\n");
		return -1;
	}

	if (adc_ch_a >= ADC_DMA_NUM_CHANNELS || adc_ch_b >= ADC_DMA_NUM_CHANNELS) {
		printf("%s: Invalid ADC channels (max %d)\n", motor->name, ADC_DMA_NUM_CHANNELS - 1);
		return -1;
	}

	motor->current_cfg.adc_channel_a = adc_ch_a;
	motor->current_cfg.adc_channel_b = adc_ch_b;
	motor->current_cfg.current_sensitivity = sensitivity;
	motor->current_cfg.current_offset = offset;
	motor->current_cfg.current_limit_a = limit_a;

	printf("%s: Current sensing configured - ch_a=%u, ch_b=%u, sens=%d mV/A, limit=%d A\n",
		motor->name, adc_ch_a, adc_ch_b, (int)(sensitivity * 1000), (int)limit_a);

	return 0;
}

int foc_current_enable(struct foc_motor *motor)
{
	uint16_t adc_raw_a, adc_raw_b;
	float voltage_a, voltage_b;

	if (!motor) {
		printf("FOC: Motor not initialized\n");
		return -1;
	}

	/* Calibrate offset by reading ADC when motor is stopped */
	/* Wait a bit for ADC to stabilize */
	HAL_Delay(50);

	if (adc_dma_get_channel(motor->current_cfg.adc_channel_a, &adc_raw_a) == 0 &&
	    adc_dma_get_channel(motor->current_cfg.adc_channel_b, &adc_raw_b) == 0) {

		voltage_a = (float)adc_dma_raw_to_mv(adc_raw_a) / 1000.0f;
		voltage_b = (float)adc_dma_raw_to_mv(adc_raw_b) / 1000.0f;

		/* Use average of both channels as offset */
		motor->current_cfg.current_offset = (voltage_a + voltage_b) / 2.0f;

		printf("%s: Current sensing enabled, calibrated offset=%dmV (A=%dmV B=%dmV)\n",
		       motor->name, (int)(motor->current_cfg.current_offset * 1000),
		       (int)(voltage_a * 1000), (int)(voltage_b * 1000));
	} else {
		printf("%s: Current sensing enabled (calibration failed, using default offset)\n",
		       motor->name);
	}

	motor->current_cfg.enabled = true;
	motor->current_data.overcurrent = false;

	return 0;
}

int foc_current_disable(struct foc_motor *motor)
{
	if (!motor) {
		printf("FOC: Motor not initialized\n");
		return -1;
	}

	motor->current_cfg.enabled = false;
	printf("%s: Current sensing disabled\n", motor->name);
	return 0;
}

void foc_current_update(struct foc_motor *motor)
{
	struct foc_current_config *cfg;
	struct foc_current_data *data;
	uint16_t adc_raw_a, adc_raw_b;
	float voltage_a, voltage_b;

	if (!motor || !motor->current_cfg.enabled) {
		return;
	}

	cfg = &motor->current_cfg;
	data = &motor->current_data;

	/* Read ADC values for phase A and B */
	if (adc_dma_get_channel(cfg->adc_channel_a, &adc_raw_a) != 0) {
		return;
	}
	if (adc_dma_get_channel(cfg->adc_channel_b, &adc_raw_b) != 0) {
		return;
	}

	/* Convert ADC to voltage */
	voltage_a = (float)adc_dma_raw_to_mv(adc_raw_a) / 1000.0f;  /* Convert mV to V */
	voltage_b = (float)adc_dma_raw_to_mv(adc_raw_b) / 1000.0f;

	/* Debug: Print raw ADC values once per second */
	static uint32_t last_debug = 0;
	uint32_t now = HAL_GetTick();
	if (now - last_debug > 1000) {
		printf("%s: ADC raw: A=%u B=%u, Voltage: A=%dmV B=%dmV, Offset=%dmV\n",
		       motor->name, adc_raw_a, adc_raw_b,
		       (int)(voltage_a * 1000), (int)(voltage_b * 1000),
		       (int)(cfg->current_offset * 1000));
		last_debug = now;
	}

	/* Convert voltage to current
	 * Current = (Voltage - Offset) / Sensitivity
	 * Hardware: INA181A1 (gain=20) + 0.01Ω shunt resistor
	 * Sensitivity = 20 × 0.01Ω = 0.2 V/A (200mV/A)
	 * Offset is auto-calibrated at enable time (~1.6V typical)
	 */
	data->phase_a_current = (voltage_a - cfg->current_offset) / cfg->current_sensitivity;
	data->phase_b_current = (voltage_b - cfg->current_offset) / cfg->current_sensitivity;

	/* Calculate phase C current using Kirchhoff's law
	 * Ia + Ib + Ic = 0, therefore Ic = -(Ia + Ib)
	 */
	data->phase_c_current = -(data->phase_a_current + data->phase_b_current);

	/* Calculate current magnitude (RMS-like)
	 * magnitude = sqrt((Ia^2 + Ib^2 + Ic^2) / 3)
	 */
	data->magnitude = sqrtf((data->phase_a_current * data->phase_a_current +
	                         data->phase_b_current * data->phase_b_current +
	                         data->phase_c_current * data->phase_c_current) / 3.0f);

	/* Check overcurrent condition - only apply protection during active control */
	if (fabsf(data->magnitude) > cfg->current_limit_a) {
		data->overcurrent = true;

		/* Only reduce amplitude if motor is actively running (velocity control active) */
		if (motor->velocity_cfg.mode != FOC_VELOCITY_DISABLED && motor->amplitude > 0.0f) {
			motor->amplitude *= 0.9f;  /* Reduce by 10% */
			if (motor->amplitude < 1.0f) {
				motor->amplitude = 0.0f;
			}
			printf("%s: Overcurrent detected (%d mA), reducing amplitude to %d%%\n",
			       motor->name, (int)(data->magnitude * 1000.0f), (int)motor->amplitude);
		}
	} else {
		data->overcurrent = false;
	}
}

int foc_current_get(struct foc_motor *motor, float *current_a)
{
	if (!motor) {
		return -1;
	}

	if (current_a) {
		*current_a = motor->current_data.magnitude;
	}

	return 0;
}

bool foc_current_is_overcurrent(struct foc_motor *motor)
{
	if (!motor) {
		return false;
	}

	return motor->current_data.overcurrent;
}

int foc_current_set_limit(struct foc_motor *motor, float limit_a)
{
	if (!motor) {
		printf("FOC: Motor not initialized\n");
		return -1;
	}

	if (limit_a < 0.1f || limit_a > 50.0f) {
		printf("%s: Invalid current limit (must be 0.1-50 A)\n", motor->name);
		return -1;
	}

	motor->current_cfg.current_limit_a = limit_a;
	printf("%s: Current limit set to %d A\n", motor->name, (int)limit_a);

	return 0;
}
