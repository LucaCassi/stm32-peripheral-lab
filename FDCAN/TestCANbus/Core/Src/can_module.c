/*
 * can_module.c
 *
 *  Created on: Nov 3, 2025
 *      Author: luccas
 */

#include "can_module.h"

#include "fdcan.h"

static uint8_t CAN_Tx_Data[8] = { 0 };
static uint8_t CAN_Rx_Data[8] = { 0 };
uint32_t can_error=0;
void HAL_FDCAN_ErrorCallback(FDCAN_HandleTypeDef *hfdcan);
FDCAN_ProtocolStatusTypeDef ps;

typedef enum{
	CAN_MODULE_FIFO_FULL,
	CAN_MODULE_ERROR_WARNING,
	CAN_MODULE_ERROR_PASSIVE,
	CAN_MODULE_ERROR_BUS_OFF,
	CAN_MODULE_ERROR_OTHERS,
	CAN_MODULE_ERROR_FW,
	CAN_MODULE_ERROR_LENGTH,
}e_can_module_error;

static uint8_t last_msg_idx;

uint32_t can_module_error[CAN_MODULE_INSTANCE_LENGTH][CAN_MODULE_ERROR_LENGTH]={0};
uint32_t j=0;

FDCAN_TxHeaderTypeDef CAN_tx_Header;
FDCAN_RxHeaderTypeDef CAN_rx_Header;
HAL_StatusTypeDef error;

uint32_t err;

static
void can_module_error_handler(FDCAN_HandleTypeDef *hfdcan);

void can_module_init(e_can_module_instance can_instance, uint32_t Identifier){

	FDCAN_HandleTypeDef *can_instance_ptr;

	if(can_instance==CAN_MODULE_FDCAN1)
		can_instance_ptr=&hfdcan1;
	else
		;
		//can_instance_ptr=&hfdcan2;

	CAN_tx_Header.BitRateSwitch = FDCAN_BRS_OFF;
	CAN_tx_Header.DataLength = 8;
	CAN_tx_Header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	CAN_tx_Header.FDFormat = FDCAN_CLASSIC_CAN;
	CAN_tx_Header.Identifier = Identifier;
	CAN_tx_Header.IdType = FDCAN_STANDARD_ID;
	CAN_tx_Header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
	CAN_tx_Header.TxFrameType = FDCAN_DATA_FRAME;
	CAN_tx_Header.MessageMarker = 0;



	HAL_FDCAN_Init(can_instance_ptr);
	HAL_FDCAN_Start(can_instance_ptr);


    uint32_t it =  FDCAN_IT_ERROR_WARNING
                 | FDCAN_IT_ERROR_PASSIVE
                 | FDCAN_IT_BUS_OFF | FDCAN_IE_TEFLE | FDCAN_IE_TEFFE | FDCAN_IT_RX_FIFO0_NEW_MESSAGE;
    // Activate the notification for new data in FIFO1 for FDCAN2
	if (HAL_FDCAN_ActivateNotification(can_instance_ptr, it, 0) != HAL_OK)
	{
	  /* Notification Error */
	  Error_Handler();
	}
}
void can_module_transmit(e_can_module_instance can_instance, uint8_t *tx_data){

	FDCAN_HandleTypeDef *can_instance_ptr;

	if(can_instance==CAN_MODULE_FDCAN1)
		can_instance_ptr=&hfdcan1;
	else
		;
		//can_instance_ptr=&hfdcan2;

		//CAN_Tx_Data[0] = j ;
		error=HAL_FDCAN_AddMessageToTxFifoQ(can_instance_ptr, &CAN_tx_Header, tx_data);
		if(error!=HAL_OK){
			can_module_error_handler(&hfdcan1);
			//HAL_Delay(10);
		}
		//j++;
		//HAL_Delay(1);

}

static
void can_module_error_handler(FDCAN_HandleTypeDef *hfdcan)
{
    e_can_module_instance instance;
    if(hfdcan==&hfdcan1)
    	instance=CAN_MODULE_FDCAN1;
    else
    	instance=CAN_MODULE_FDCAN2;

	can_error=hfdcan->ErrorCode;
	if(can_error==0x200)
		can_module_error[instance][CAN_MODULE_FIFO_FULL]++;
	else
		can_module_error[instance][CAN_MODULE_ERROR_OTHERS]++;
}


void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t ErrorStatusITs)
{
    HAL_FDCAN_GetProtocolStatus(hfdcan, &ps);
    //err_reg = HAL_FDCAN_GetError(hfdcan);
    e_can_module_instance instance;
    if(hfdcan==&hfdcan1)
    	instance=CAN_MODULE_FDCAN1;
    else
    	instance=CAN_MODULE_FDCAN2;

    if (ps.Warning) {
        // happens soon after unplug (no ACK -> TEC/REC >= 96)
		can_module_error[instance][CAN_MODULE_ERROR_WARNING]++;
    }
    if (ps.ErrorPassive) {
        // escalation
		can_module_error[instance][CAN_MODULE_ERROR_PASSIVE]++;
    }
    if (ps.BusOff) {
        // bus-off policy (auto or manual recovery)
		can_module_error[instance][CAN_MODULE_ERROR_BUS_OFF]++;
    }
    if (!ps.BusOff && !ps.ErrorPassive && !ps.Warning){
		can_module_error[instance][CAN_MODULE_ERROR_OTHERS]++;
    }
}
uint8_t err_num=0;
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    e_can_module_instance instance;
    if(hfdcan==&hfdcan1)
    	instance=CAN_MODULE_FDCAN1;
    else
    	instance=CAN_MODULE_FDCAN2;

    HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &CAN_rx_Header, CAN_Rx_Data);
    if(CAN_Rx_Data[0]!=last_msg_idx+1 ){
    	if(last_msg_idx==255 && CAN_Rx_Data[0]==0){
    	}else{
    		can_module_error[instance][CAN_MODULE_ERROR_FW] += 1;
        	err_num=CAN_Rx_Data[0];
    	}
    }
    last_msg_idx=CAN_Rx_Data[0];
}
