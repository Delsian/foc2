#ifndef __CANOPEN_H
#define __CANOPEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32g4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* NMT States */
typedef enum {
    NMT_INITIALIZING    = 0x00,
    NMT_STOPPED         = 0x04,
    NMT_OPERATIONAL     = 0x05,
    NMT_PRE_OPERATIONAL = 0x7F
} NMT_State;

/* Public API */
void canopen_init(void);
void canopen_process(void);
void canopen_handle_rx(uint32_t id, uint8_t* data, uint8_t len);

/* Accessors for application */
NMT_State canopen_get_state(void);

#ifdef __cplusplus
}
#endif

#endif /* __CANOPEN_H */
