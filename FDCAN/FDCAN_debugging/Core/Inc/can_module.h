/**
  ******************************************************************************
  * @file           : can_module.h
  * @author         : Luca Cassi
  ******************************************************************************
  * @brief          : Minimal CAN (FDCAN) helper module.
  *
  * This module was used as a lightweight debugging aid on a custom board:
  *  - provides a simple init + transmit API
  *  - counts controller/bus error conditions via HAL callbacks
  *  - performs a basic RX sequence check on byte[0] of received frames
  *
  * Notes:
  *  - The current implementation effectively targets FDCAN1 only (FDCAN2 is
  *    present in the enum but not wired in the code).
  ******************************************************************************
*/

#ifndef INC_CAN_MODULE_H_
#define INC_CAN_MODULE_H_

#include <stdint.h>
#include "stm32g4xx_hal.h"

/** Select which FDCAN peripheral instance to use. */
typedef enum{
	CAN_MODULE_FDCAN1,
	CAN_MODULE_FDCAN2,
	CAN_MODULE_INSTANCE_LENGTH
} e_can_module_instance;

/**
 * @brief Initialize and start the selected FDCAN instance.
 * @param can_instance  FDCAN instance selector (FDCAN1 / FDCAN2).
 * @param Identifier    Standard 11-bit CAN identifier to be used for TX frames.
 */
void can_module_init(e_can_module_instance can_instance, uint32_t Identifier);

/**
 * @brief Transmit one Classic CAN data frame (DLC=8) using the configured TX header.
 * @param can_instance  FDCAN instance selector.
 * @param tx_data       Pointer to 8-byte payload.
 */
void can_module_transmit(e_can_module_instance can_instance, uint8_t *tx_data);

/**
 * @brief HAL callback for FDCAN error/status interrupts.
 *        Increments internal counters according to controller state.
 */
void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t ErrorStatusITs);

#endif /* INC_CAN_MODULE_H_ */
