#ifndef I2C_SCAN_H
#define I2C_SCAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void i2c_scan(I2C_HandleTypeDef *hi2c, const char *bus_name);

#ifdef __cplusplus
}
#endif

#endif
