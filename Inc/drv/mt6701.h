#ifndef MT6701_H
#define MT6701_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * @brief MT6701 magnetic angle encoder driver
 *
 * This driver provides I2C communication with the MT6701 14-bit
 * magnetic angle position encoder sensor.
 */

/* MT6701 I2C address (7-bit address) */
#define MT6701_I2C_ADDR            0x06

/* MT6701 register addresses */
#define MT6701_REG_ANGLE_H         0x03  /* Angle[13:6] */
#define MT6701_REG_ANGLE_L         0x04  /* Angle[5:0] + status bits */

/* Angle resolution */
#define MT6701_ANGLE_RESOLUTION    16384  /* 14-bit: 2^14 */

/**
 * @brief MT6701 device structure
 */
typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t i2c_addr;
    const char *name;
} mt6701_t;

/**
 * @brief Initialize MT6701 device
 *
 * @param dev Pointer to MT6701 device structure
 * @param hi2c Pointer to I2C handle
 * @param addr I2C address (7-bit)
 * @param name Device name for logging
 * @return 0 on success, negative value on failure
 */
int mt6701_init(mt6701_t *dev, I2C_HandleTypeDef *hi2c, uint8_t addr, const char *name);

/**
 * @brief Read raw angle value from MT6701
 *
 * @param dev Pointer to MT6701 device
 * @param angle Pointer to store 14-bit angle value (0-16383)
 * @return 0 on success, negative value on failure
 */
int mt6701_read_angle_raw(mt6701_t *dev, uint16_t *angle);

/**
 * @brief Read angle in degrees from MT6701
 *
 * @param dev Pointer to MT6701 device
 * @param angle_deg Pointer to store angle in degrees (0.0-360.0)
 * @return 0 on success, negative value on failure
 */
int mt6701_read_angle_deg(mt6701_t *dev, float *angle_deg);

/**
 * @brief Read angle in radians from MT6701
 *
 * @param dev Pointer to MT6701 device
 * @param angle_rad Pointer to store angle in radians (0.0-2π)
 * @return 0 on success, negative value on failure
 */
int mt6701_read_angle_rad(mt6701_t *dev, float *angle_rad);

#ifdef __cplusplus
}
#endif

#endif /* MT6701_H */
