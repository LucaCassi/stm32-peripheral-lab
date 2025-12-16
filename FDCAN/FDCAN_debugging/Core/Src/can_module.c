/**
  ******************************************************************************
  * @file           : can_module.c
  * @author         : Luca Cassi
  ******************************************************************************
  * @brief          : Implementation of minimal CAN (FDCAN) helper module.
  *
  * Design intent:
  *  - Keep the code small and easy to drop into a debug firmware.
  *  - Collect error statistics to diagnose intermittent CAN issues.
  *  - Detect missing/out-of-order frames by checking a sequence counter in RX byte[0].
  *
  * Assumptions:
  *  - Classic CAN (not CAN FD) frames are used (FDFormat = Classic, BRS off).
  *  - DLC is fixed to 8 bytes.
  *  - Rx is handled through FIFO0 callback.
  ******************************************************************************
*/

#include "can_module.h"
#include "fdcan.h"

/* Local buffers used by callbacks (RX) and potential debug (TX). */
static uint8_t CAN_Tx_Data[8] = { 0 }; /* Currently unused: tx_data is passed directly. */
static uint8_t CAN_Rx_Data[8] = { 0 };

/* Last HAL error code captured for quick inspection while debugging. */
uint32_t can_error = 0;

/* Protocol status snapshot used inside the ErrorStatus callback. */
FDCAN_ProtocolStatusTypeDef ps;

/**
 * Error categories counted by this module:
 *  - FIFO_FULL:     TX FIFO/Queue full when trying to enqueue a frame.
 *  - WARNING:       Controller entered Warning level (TEC/REC >= threshold).
 *  - PASSIVE:       Controller entered Error Passive state.
 *  - BUS_OFF:       Controller entered Bus-Off state.
 *  - OTHERS:        Any other error/status condition not in the above buckets.
 *  - FW:            Application-level error (RX sequence discontinuity).
 */
typedef enum{
	CAN_MODULE_FIFO_FULL,
	CAN_MODULE_ERROR_WARNING,
	CAN_MODULE_ERROR_PASSIVE,
	CAN_MODULE_ERROR_BUS_OFF,
	CAN_MODULE_ERROR_OTHERS,
	CAN_MODULE_ERROR_FW,
	CAN_MODULE_ERROR_LENGTH,
} e_can_module_error;

/* Used to check RX sequence on CAN_Rx_Data[0]. */
static uint8_t last_msg_idx;

/* Public counters: [instance][error_type]. */
uint32_t can_module_error[CAN_MODULE_INSTANCE_LENGTH][CAN_MODULE_ERROR_LENGTH] = {0};

/* Debug variables. */
FDCAN_TxHeaderTypeDef CAN_tx_Header;
FDCAN_RxHeaderTypeDef CAN_rx_Header;
HAL_StatusTypeDef error;

/* Internal helper to classify HAL enqueue failures. */
static void can_module_error_handler(FDCAN_HandleTypeDef *hfdcan);

void can_module_init(e_can_module_instance can_instance, uint32_t Identifier)
{
	FDCAN_HandleTypeDef *can_instance_ptr;

	/* Select the FDCAN peripheral handle. */
	if (can_instance == CAN_MODULE_FDCAN1)
		can_instance_ptr = &hfdcan1;
	else
		;
		//can_instance_ptr = &hfdcan2;

	/* Configure a fixed TX header (Classic CAN, DLC=8, Standard ID). */
	CAN_tx_Header.BitRateSwitch = FDCAN_BRS_OFF;
	CAN_tx_Header.DataLength = 8;
	CAN_tx_Header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	CAN_tx_Header.FDFormat = FDCAN_CLASSIC_CAN;
	CAN_tx_Header.Identifier = Identifier;
	CAN_tx_Header.IdType = FDCAN_STANDARD_ID;
	CAN_tx_Header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
	CAN_tx_Header.TxFrameType = FDCAN_DATA_FRAME;
	CAN_tx_Header.MessageMarker = 0;

	/* Initialize and start the peripheral. */
	HAL_FDCAN_Init(can_instance_ptr);
	HAL_FDCAN_Start(can_instance_ptr);

	/* Enable interrupts/notifications used for debugging and statistics. */
	uint32_t it =
		  FDCAN_IT_ERROR_WARNING
		| FDCAN_IT_ERROR_PASSIVE
		| FDCAN_IT_BUS_OFF
		| FDCAN_IE_TEFLE
		| FDCAN_IE_TEFFE
		| FDCAN_IT_RX_FIFO0_NEW_MESSAGE;

	/* Activate notifications (3rd parameter is Rx FIFO watermark / unused here). */
	if (HAL_FDCAN_ActivateNotification(can_instance_ptr, it, 0) != HAL_OK)
	{
		/* Notification Error */
		Error_Handler();
	}
}

void can_module_transmit(e_can_module_instance can_instance, uint8_t *tx_data)
{
	FDCAN_HandleTypeDef *can_instance_ptr;

	/* Select the FDCAN peripheral handle. */
	if (can_instance == CAN_MODULE_FDCAN1)
		can_instance_ptr = &hfdcan1;
	else
		;
		//can_instance_ptr = &hfdcan2;

	/* Try to enqueue a frame in the TX FIFO/Queue. */
	error = HAL_FDCAN_AddMessageToTxFifoQ(can_instance_ptr, &CAN_tx_Header, tx_data);

	/* If enqueue failed, classify and count the error. */
	if (error != HAL_OK)
	{
		/* NOTE: currently hard-coded to hfdcan1 (kept as-is). */
		can_module_error_handler(&hfdcan1);
		//HAL_Delay(10);
	}
}

/**
 * @brief Classify HAL enqueue errors using hfdcan->ErrorCode and increment counters.
 *
 * NOTE:
 *  - Uses a magic value (0x200) to detect a "FIFO full" condition.
 *  - Everything else is counted as "OTHERS".
 */
static void can_module_error_handler(FDCAN_HandleTypeDef *hfdcan)
{
	e_can_module_instance instance;

	if (hfdcan == &hfdcan1)
		instance = CAN_MODULE_FDCAN1;
	else
		instance = CAN_MODULE_FDCAN2;

	can_error = hfdcan->ErrorCode;

	if (can_error == 0x200)
		can_module_error[instance][CAN_MODULE_FIFO_FULL]++;
	else
		can_module_error[instance][CAN_MODULE_ERROR_OTHERS]++;
}

/**
 * @brief HAL callback called on FDCAN error/status interrupt.
 *
 * This callback reads the controller protocol status and increments counters:
 *  - Warning: triggered when the node is not ACKed (e.g., bus unplugged).
 *  - ErrorPassive: escalation of error state.
 *  - BusOff: controller disconnected itself from the bus.
 */
void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t ErrorStatusITs)
{
	(void)ErrorStatusITs; /* Currently unused: status is read from protocol status. */

	HAL_FDCAN_GetProtocolStatus(hfdcan, &ps);

	e_can_module_instance instance;

	if (hfdcan == &hfdcan1)
		instance = CAN_MODULE_FDCAN1;
	else
		instance = CAN_MODULE_FDCAN2;

	if (ps.Warning)
	{
		/* Happens soon after unplug (no ACK -> TEC/REC >= 96). */
		can_module_error[instance][CAN_MODULE_ERROR_WARNING]++;
	}
	if (ps.ErrorPassive)
	{
		/* Escalation. */
		can_module_error[instance][CAN_MODULE_ERROR_PASSIVE]++;
	}
	if (ps.BusOff)
	{
		/* Bus-off policy (auto or manual recovery). */
		can_module_error[instance][CAN_MODULE_ERROR_BUS_OFF]++;
	}
	if (!ps.BusOff && !ps.ErrorPassive && !ps.Warning)
	{
		can_module_error[instance][CAN_MODULE_ERROR_OTHERS]++;
	}
}

/* Last received sequence number captured on mismatch (debug aid). */
uint8_t err_num = 0;

/**
 * @brief HAL callback called when a new message is available in RX FIFO0.
 *
 * The first byte of the payload (byte[0]) is treated as a sequence counter.
 * This detects missing/out-of-order frames at application level.
 */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
	(void)RxFifo0ITs; /* Currently unused: always read one message from FIFO0. */

	e_can_module_instance instance;

	if (hfdcan == &hfdcan1)
		instance = CAN_MODULE_FDCAN1;
	else
		instance = CAN_MODULE_FDCAN2;

	/* Pop one message from FIFO0 into CAN_rx_Header and CAN_Rx_Data. */
	HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &CAN_rx_Header, CAN_Rx_Data);

	/* Sequence continuity check with wrap-around 255 -> 0. */
	if (CAN_Rx_Data[0] != (uint8_t)(last_msg_idx + 1))
	{
		if (last_msg_idx == 255 && CAN_Rx_Data[0] == 0)
		{
			/* Expected wrap-around, do nothing. */
		}
		else
		{
			/* Sequence mismatch: count as application-level FW error. */
			can_module_error[instance][CAN_MODULE_ERROR_FW] += 1;
			err_num = CAN_Rx_Data[0];
		}
	}

	last_msg_idx = CAN_Rx_Data[0];
}
