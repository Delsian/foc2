#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "stm32g4xx_hal.h"
#include "stm32g4xx_ll_ucpd.h"
#include "stm32g4xx_ll_bus.h"
#include "stm32g4xx_ll_cortex.h"
#include "stm32g4xx_ll_rcc.h"
#include "stm32g4xx_ll_system.h"
#include "stm32g4xx_ll_utils.h"
#include "stm32g4xx_ll_pwr.h"
#include "stm32g4xx_ll_gpio.h"
#include "stm32g4xx_ll_dma.h"
#include "stm32g4xx_ll_exti.h"

void Error_Handler(void);
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_DMA_Init(void);
void MX_ADC2_Init(void);
void MX_FDCAN1_Init(void);
void MX_I2C1_Init(void);
void MX_I2C2_Init(void);
void MX_TIM2_Init(void);
void MX_TIM3_Init(void);
void MX_TIM4_Init(void);
void MX_UCPD1_Init(void);
void MX_USART2_UART_Init(void);


typedef enum {
    CMD_RESET = 0x01,
    CMD_ADC = 0x02,
    CMD_PWM = 0x04,  /* Periodical PWM task */
    CMD_VEL_INC = 0x10,
    CMD_VEL_DEC = 0x20,
    CMD_AMP_INC = 0x40,
    CMD_AMP_DEC = 0x80,
    CMD_TOGGLE_MODE = 0x100,
    CMD_TOGGLE_ENCODER = 0x200,
    CMD_CALIBRATE = 0x400,
    CMD_INFO = 0x800,
    CMD_HELP = 0x1000,
} MainCommands;

void set_event(MainCommands cmd);

#include "foc.h"
#include "drv/pwm.h"
#include "drv/mt6701.h"

extern struct foc_motor *motor[2];
extern struct pwm_device *pwm_dev[2];
extern mt6701_t encoder_motor1;
extern bool encoder_enabled[2];

extern float angle;
extern float target_rpm;
extern float amplitude;
extern bool velocity_mode;

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
