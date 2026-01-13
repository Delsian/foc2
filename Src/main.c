#include "main.h"
#include "usb_device.h"
#include "drv/i2c_scan.h"
#include "drv/uart_in.h"
#include "drv/adc_dma.h"
#include "drv/can.h"
#include "drv/pwm.h"
#include "drv/mt6701.h"
#include "foc.h"
#include <stdio.h>

ADC_HandleTypeDef hadc2;
DMA_HandleTypeDef hdma_adc2;
FDCAN_HandleTypeDef hfdcan1;
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;
UART_HandleTypeDef huart2;

static MainCommands command = 0;
static struct pwm_device *pwm_dev[2];
static struct foc_motor *motor[2];
static mt6701_t encoder_motor0;
static mt6701_t encoder_motor1;

/* Motor control state variables */
static float angle = 0.0f;
static float target_rpm = 0.0f;
static float amplitude = 5.0f;  /* Start with low amplitude to prevent overcurrent */
static bool velocity_mode = false;

static void init(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_DMA_Init();
    MX_ADC2_Init();
    MX_FDCAN1_Init();
    MX_I2C1_Init();
    MX_I2C2_Init();
    MX_TIM2_Init();
    MX_TIM3_Init();
    MX_TIM4_Init();
    MX_UCPD1_Init();
    MX_USART2_UART_Init();
    MX_USB_Device_Init();
    MX_FDCAN1_Init();

    adc_dma_init(&hadc2, &hdma_adc2, &htim2);

    uart_in_init(&huart2);
    can_init(&hfdcan1);

    /* Initialize MT6701 encoders */
    mt6701_init(&encoder_motor0, &hi2c2, MT6701_I2C_ADDR, "encoder_motor0");
    mt6701_init(&encoder_motor1, &hi2c1, MT6701_I2C_ADDR, "encoder_motor1");
}

static void pwm_init_devices(void)
{
    pwm_dev[0] = pwm_get_device("pwm_motor0");
    pwm_dev[1] = pwm_get_device("pwm_motor1");

    for (int i = 0; i < 2; i++) {
        if (pwm_dev[i]) {
            pwm_init(pwm_dev[i]);
        }
    }

    /* Initialize FOC motor instances */
    motor[0] = foc_get_motor("motor0");
    motor[1] = foc_get_motor("motor1");
}

void set_event(MainCommands cmd)
{
    command |= cmd;
}

/**
 * @brief Timer period elapsed callback
 * Called from TIM4 interrupt at 1kHz
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM4) {
        /* Set event to trigger PWM velocity control update at 1kHz */
        set_event(CMD_PWM);
    }
}

int main(void)
+{
    static uint32_t cnt = 0;

    init();
    pwm_init_devices();

    /* Start PWM on both motors */
    pwm_start(pwm_dev[0]);
    pwm_start(pwm_dev[1]);

    /* Start TIM4 interrupt for velocity control at 1kHz */
    HAL_TIM_Base_Start_IT(&htim4);

    printf("hello\n");
    printf("Commands:\n");
    printf("  + : Increase velocity by 10 RPM\n");
    printf("  - : Decrease velocity by 10 RPM\n");
    printf("  > : Increase amplitude by 5%%\n");
    printf("  < : Decrease amplitude by 5%%\n");
    printf("  p : Toggle position/velocity mode\n");
    printf("  i : Print info\n");

    i2c_scan(&hi2c1, "I2C1");
    i2c_scan(&hi2c2, "I2C2");

    adc_dma_start();

    /* Enable current sensing for motor1 (after ADC DMA is started) */
    foc_current_enable(motor[1]);

    while (1) {
        if (uart_in_available() > 0) {
            uint8_t ch;
            uart_in_getchar(&ch);

            switch (ch) {
                case '+':
                    /* Increase velocity by 10 RPM */
                    target_rpm += 10.0f;
                    if (target_rpm > 500.0f) target_rpm = 500.0f;  /* Limit max RPM */

                    if (!velocity_mode) {
                        /* Enable velocity mode on first velocity command */
                        foc_velocity_enable(motor[0], FOC_VELOCITY_OPEN_LOOP,
                                          target_rpm, amplitude, 1000.0f, 7);
                        foc_velocity_enable(motor[1], FOC_VELOCITY_OPEN_LOOP,
                                          target_rpm, amplitude, 1000.0f, 7);
                        velocity_mode = true;
                        printf("Velocity mode enabled (amplitude: %d%%)\n", (int)amplitude);
                    } else {
                        foc_velocity_set_target(motor[0], target_rpm);
                        foc_velocity_set_target(motor[1], target_rpm);
                    }
                    printf("Target velocity: %d RPM\n", (int)target_rpm);
                    break;

                case '-':
                    /* Decrease velocity by 10 RPM */
                    target_rpm -= 10.0f;
                    if (target_rpm < -500.0f) target_rpm = -500.0f;  /* Limit min RPM */

                    if (!velocity_mode) {
                        /* Enable velocity mode on first velocity command */
                        foc_velocity_enable(motor[0], FOC_VELOCITY_OPEN_LOOP,
                                          target_rpm, amplitude, 1000.0f, 7);
                        foc_velocity_enable(motor[1], FOC_VELOCITY_OPEN_LOOP,
                                          target_rpm, amplitude, 1000.0f, 7);
                        velocity_mode = true;
                        printf("Velocity mode enabled (amplitude: %d%%)\n", (int)amplitude);
                    } else {
                        foc_velocity_set_target(motor[0], target_rpm);
                        foc_velocity_set_target(motor[1], target_rpm);
                    }
                    printf("Target velocity: %d RPM\n", (int)target_rpm);
                    break;

                case '>':
                    /* Increase amplitude by 5% */
                    amplitude += 5.0f;
                    if (amplitude > 100.0f) amplitude = 100.0f;
                    printf("Amplitude: %d%%\n", (int)amplitude);

                    /* Update amplitude if in velocity mode */
                    if (velocity_mode) {
                        foc_velocity_disable(motor[0]);
                        foc_velocity_disable(motor[1]);
                        foc_velocity_enable(motor[0], FOC_VELOCITY_OPEN_LOOP,
                                          target_rpm, amplitude, 1000.0f, 7);
                        foc_velocity_enable(motor[1], FOC_VELOCITY_OPEN_LOOP,
                                          target_rpm, amplitude, 1000.0f, 7);
                    }
                    break;

                case '<':
                    /* Decrease amplitude by 5% */
                    amplitude -= 5.0f;
                    if (amplitude < 0.0f) amplitude = 0.0f;
                    printf("Amplitude: %d%%\n", (int)amplitude);

                    /* Update amplitude if in velocity mode */
                    if (velocity_mode) {
                        foc_velocity_disable(motor[0]);
                        foc_velocity_disable(motor[1]);
                        foc_velocity_enable(motor[0], FOC_VELOCITY_OPEN_LOOP,
                                          target_rpm, amplitude, 1000.0f, 7);
                        foc_velocity_enable(motor[1], FOC_VELOCITY_OPEN_LOOP,
                                          target_rpm, amplitude, 1000.0f, 7);
                    }
                    break;

                case 'p':
                case 'P':
                    /* Toggle position/velocity mode */
                    if (velocity_mode) {
                        foc_velocity_disable(motor[0]);
                        foc_velocity_disable(motor[1]);
                        velocity_mode = false;
                        target_rpm = 0.0f;
                        printf("Position mode enabled\n");
                    } else {
                        foc_velocity_enable(motor[0], FOC_VELOCITY_OPEN_LOOP,
                                          target_rpm, amplitude, 1000.0f, 7);
                        foc_velocity_enable(motor[1], FOC_VELOCITY_OPEN_LOOP,
                                          target_rpm, amplitude, 1000.0f, 7);
                        velocity_mode = true;
                        printf("Velocity mode enabled (amplitude: %d%%)\n", (int)amplitude);
                    }
                    break;

                case 'i':
                case 'I':
                    /* Print info */
                    printf("\n=== Motor Control Info ===\n");
                    printf("Mode: %s\n", velocity_mode ? "Velocity" : "Position");
                    printf("Amplitude: %d%%\n", (int)amplitude);

                    if (velocity_mode) {
                        float rpm0, rpm1;
                        foc_velocity_get_current(motor[0], &rpm0);
                        foc_velocity_get_current(motor[1], &rpm1);
                        printf("Target RPM: %d\n", (int)target_rpm);
                        printf("Motor 0 current RPM: %d\n", (int)rpm0);
                        printf("Motor 1 current RPM: %d\n", (int)rpm1);
                    } else {
                        printf("Position angle: %d deg\n", (int)angle);
                    }

                    /* Print current sensing info */
                    float current_a = 0.0f;
                    foc_current_get(motor[1], &current_a);
                    printf("Motor 1 current: %d mA (limit: %d A)\n",
                           (int)(current_a * 1000),
                           (int)motor[1]->current_cfg.current_limit_a);

                    /* Read encoder angles */
                    float angle0, angle1;
                    if (mt6701_read_angle_deg(&encoder_motor0, &angle0) == 0 &&
                        mt6701_read_angle_deg(&encoder_motor1, &angle1) == 0) {
                        printf("Encoder 0: %d deg\n", (int)angle0);
                        printf("Encoder 1: %d deg\n", (int)angle1);
                    }
                    printf("========================\n\n");
                    break;

                default:
                    /* In position mode, use angle control */
                    if (!velocity_mode) {
                        angle += 10.0f;
                        if (angle >= 360.0f) {
                            angle -= 360.0f;
                        }
                        pwm_set_vector(pwm_dev[0], angle, amplitude);
                        pwm_set_vector(pwm_dev[1], angle, amplitude);
                        printf("Position: %d deg (amplitude: %d%%)\n", (int)angle, (int)amplitude);
                    }
                    break;
            }
        }

        if (command & CMD_RESET) {
            command &= ~CMD_RESET;
            NVIC_SystemReset();
        }

        if (command & CMD_ADC) {
            command &= ~CMD_ADC;

            // Handle ADC command
        }

        if (cnt++ >= 350000) {
            cnt = 0;
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_6);

            /* ADC testing */
            //uint16_t adc_values[5];
            //adc_dma_get_all_channels(adc_values, 5);
            // printf("ADC Values: %u %u %u %u %u\n",
            //        adc_values[0], adc_values[1],
            //        adc_values[2], adc_values[3],
            //        adc_values[4]);

            /* MT6701 testing */
            // float angle0, angle1;
            // mt6701_read_angle_deg(&encoder_motor0, &angle0);
            // mt6701_read_angle_deg(&encoder_motor1, &angle1);
            // printf("Encoder angles: motor0=%d deg, motor1=%d deg\n", (int)angle0, (int)angle1);

            /* Send CAN frame with ADC1 value */
            // uint8_t can_data[2];
            // can_data[0] = (adc_values[0] >> 8) & 0xFF;  /* ADC1 high byte */
            // can_data[1] = adc_values[0] & 0xFF;         /* ADC1 low byte */
            // can_transmit(0x100, can_data, 2);

        }

        if (command & CMD_PWM) {
            command &= ~CMD_PWM;
            foc_task();
        }
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
