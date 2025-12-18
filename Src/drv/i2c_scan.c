#include "drv/i2c_scan.h"
#include <stdio.h>

void i2c_scan(I2C_HandleTypeDef *hi2c, const char *bus_name)
{
    uint8_t dev_count = 0;

    printf("Scanning %s...\n", bus_name);

    for (uint8_t addr = 1; addr < 128; addr++) {
        if (HAL_I2C_IsDeviceReady(hi2c, addr << 1, 1, 10) == HAL_OK) {
            printf("  Found device at 0x%02X\n", addr);
            dev_count++;
        }
    }

    if (dev_count == 0) {
        printf("  No devices found\n");
    } else {
        printf("  Total: %d device(s)\n", dev_count);
    }
}
