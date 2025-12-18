#ifndef OLED_H
#define OLED_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void oled_init(void);
void oled_clear(void);
void oled_write(const char *str, uint8_t x, uint8_t y);
void oled_update(void);

#ifdef __cplusplus
}
#endif

#endif
