#ifndef __CAN_H
#define __CAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32g4xx_hal.h"

void can_init(FDCAN_HandleTypeDef *hfdcan);
void can_transmit(uint32_t id, uint8_t *data, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* __CAN_H */
