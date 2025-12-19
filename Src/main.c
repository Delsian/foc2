#include "main.h"
#include "usb_device.h"
#include "drv/oled.h"
#include "drv/i2c_scan.h"
#include "drv/uart_in.h"
#include "drv/adc_dma.h"
#include <stdio.h>

ADC_HandleTypeDef hadc2;
DMA_HandleTypeDef hdma_adc2;
FDCAN_HandleTypeDef hfdcan1;
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
UART_HandleTypeDef huart2;

static MainCommands command = 0;

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
    MX_UCPD1_Init();
    MX_USART2_UART_Init();
    MX_USB_Device_Init();

    adc_dma_init(&hadc2, &hdma_adc2, &htim2);

    oled_init(&hi2c1);
    uart_in_init(&huart2);
}

int main(void)
{
    static uint32_t cnt = 0;

    init();

    printf("hello\n");

    i2c_scan(&hi2c1, "I2C1");
    i2c_scan(&hi2c2, "I2C2");

    oled_write("1234567", 0, 0);
    oled_write("6677889", 0, 2);
    oled_write("6677889", 0, 4);
    oled_update();

    adc_dma_start();

    while (1) {
        if (uart_in_available() > 0) {
            uint8_t ch;
            uart_in_getchar(&ch);
            printf("Received char: %c (0x%02X)\n", ch, ch);
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
            uint16_t adc_values[5];
            adc_dma_get_all_channels(adc_values, 5);
            printf("ADC Values: %u %u %u %u %u\n",
                   adc_values[0], adc_values[1],
                   adc_values[2], adc_values[3],
                   adc_values[4]);
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
