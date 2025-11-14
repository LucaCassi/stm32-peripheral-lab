/*
 * can_module.h
 *
 *  Created on: Nov 3, 2025
 *      Author: luccas
 */

#ifndef INC_CAN_MODULE_H_
#define INC_CAN_MODULE_H_

#include <stdint.h>

#include "stm32g4xx_hal.h"

typedef enum{
	CAN_MODULE_FDCAN1,
	CAN_MODULE_FDCAN2,
	CAN_MODULE_INSTANCE_LENGTH
}e_can_module_instance;

void can_module_init(e_can_module_instance can_instance, uint32_t Identifier);

void can_module_transmit(e_can_module_instance can_instance, uint8_t *tx_data);

void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t ErrorStatusITs);

#endif /* INC_CAN_MODULE_H_ */
