#include "drv/mt6701.h"
#include <stdio.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int mt6701_init(mt6701_t *dev, I2C_HandleTypeDef *hi2c, uint8_t addr, const char *name)
{
    HAL_StatusTypeDef ret;
    uint8_t test_data;

    if (!dev || !hi2c) {
        return -1;
    }

    dev->hi2c = hi2c;
    dev->i2c_addr = addr;
    dev->name = name;

    /* Try to read a register to verify device is present and responding */
    ret = HAL_I2C_Mem_Read(dev->hi2c, dev->i2c_addr << 1, MT6701_REG_ANGLE_H,
                          I2C_MEMADD_SIZE_8BIT, &test_data, 1, 100);
    if (ret != HAL_OK) {
        printf("%s: Device not responding on I2C (error %d) - check hardware/mode\n",
               dev->name, ret);
        /* Don't fail init - device might not be connected yet */
    } else {
        printf("%s: MT6701 initialized on I2C addr 0x%02X (test read: 0x%02X)\n",
               dev->name, dev->i2c_addr, test_data);
    }

    return 0;
}

int mt6701_read_angle_raw(mt6701_t *dev, uint16_t *angle)
{
    HAL_StatusTypeDef ret;
    uint8_t data[2];

    if (!dev || !angle) {
        return -1;
    }

    /* Read angle high byte (register 0x03) */
    ret = HAL_I2C_Mem_Read(dev->hi2c, dev->i2c_addr << 1, MT6701_REG_ANGLE_H,
                          I2C_MEMADD_SIZE_8BIT, &data[0], 1, 100);
    if (ret != HAL_OK) {
        printf("%s: Failed to read angle high byte: %d\n", dev->name, ret);
        return -1;
    }

    /* Read angle low byte (register 0x04) */
    ret = HAL_I2C_Mem_Read(dev->hi2c, dev->i2c_addr << 1, MT6701_REG_ANGLE_L,
                          I2C_MEMADD_SIZE_8BIT, &data[1], 1, 100);
    if (ret != HAL_OK) {
        printf("%s: Failed to read angle low byte: %d\n", dev->name, ret);
        return -1;
    }

    /* Combine into 14-bit angle value
     * Register 0x03: Angle[13:6]
     * Register 0x04: Angle[5:0] in bits [7:2], bits [1:0] are status
     */
    *angle = ((uint16_t)data[0] << 6) | ((data[1] >> 2) & 0x3F);

    return 0;
}

int mt6701_read_angle_deg(mt6701_t *dev, float *angle_deg)
{
    uint16_t raw_angle;
    int ret;

    if (!dev || !angle_deg) {
        return -1;
    }

    ret = mt6701_read_angle_raw(dev, &raw_angle);
    if (ret < 0) {
        return ret;
    }

    /* Convert 14-bit value (0-16383) to degrees (0-360)
     * Formula from datasheet: angle = (raw * 360) / 16384
     */
    *angle_deg = ((float)raw_angle * 360.0f) / (float)MT6701_ANGLE_RESOLUTION;

    return 0;
}

int mt6701_read_angle_rad(mt6701_t *dev, float *angle_rad)
{
    uint16_t raw_angle;
    int ret;

    if (!dev || !angle_rad) {
        return -1;
    }

    ret = mt6701_read_angle_raw(dev, &raw_angle);
    if (ret < 0) {
        return ret;
    }

    /* Convert 14-bit value (0-16383) to radians (0-2π)
     * Formula: angle = (raw * 2π) / 16384
     */
    *angle_rad = ((float)raw_angle * 2.0f * (float)M_PI) / (float)MT6701_ANGLE_RESOLUTION;

    return 0;
}
