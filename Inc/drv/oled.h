#ifndef OLED_H
#define OLED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>

/* Special glyph characters */
#define OLED_GLYPH_RPM    'R'
#define OLED_GLYPH_ANGLE  'A'

void oled_init(I2C_HandleTypeDef *hi2c);
void oled_clear(void);
void oled_write(const char *str, uint8_t x, uint8_t y);
void oled_update(void);

#ifdef __cplusplus
}
#endif

#endif
