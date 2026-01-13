#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

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
} MainCommands;

void set_event(MainCommands cmd);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
