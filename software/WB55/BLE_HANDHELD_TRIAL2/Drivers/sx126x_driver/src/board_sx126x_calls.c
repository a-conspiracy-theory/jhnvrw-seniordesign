#include "board_sx126x_calls.h"
#include "main.h"

sx126x_chip_status_t radio_status;
sx126x_irq_mask_t irq_status = 0;								//set the interrupt flag to 0 at start
sx126x_status_t result = SX126X_STATUS_UNSUPPORTED_FEATURE;		//set to this so I can verify it changes later in code
sx126x_errors_mask_t errors;
uint32_t freq;


sx126x_status_t setup_22dBm_Tx(const void* context){
	if(sx126x_hal_reset(NULL) 																					!= SX126X_HAL_STATUS_OK)	{return SX126X_STATUS_ERROR;}
	if(sx126x_clear_device_errors(NULL) 																		!= SX126X_STATUS_OK)		{return SX126X_STATUS_ERROR;}
	//sx126x_set_dio2_as_rf_sw_ctrl(NULL, true);
	if(sx126x_set_standby(NULL, SX126X_STANDBY_CFG_RC) 															!= SX126X_STATUS_OK) 		{return SX126X_STATUS_ERROR;}
	if(sx126x_set_dio3_as_tcxo_ctrl(NULL, SX126X_TCXO_CTRL_1_8V, sx126x_convert_timeout_in_ms_to_rtc_step(5)) 	!= SX126X_STATUS_OK) 		{return SX126X_STATUS_ERROR;}
	if(sx126x_set_reg_mode(NULL, SX126X_REG_MODE_DCDC) 															!= SX126X_STATUS_OK)		{return SX126X_STATUS_ERROR;}
	if(sx126x_set_pkt_type(NULL, SX126X_PKT_TYPE_LORA) 															!= SX126X_STATUS_OK) 		{return SX126X_STATUS_ERROR;}
	if(sx126x_set_rf_freq(NULL, 915000000) 																		!= SX126X_STATUS_OK) 		{return SX126X_STATUS_ERROR;}
	if(sx126x_cal_img(NULL, 0xE1, 0xE9) 																		!= SX126X_STATUS_OK) 		{return SX126X_STATUS_ERROR;}

	sx126x_pa_cfg_params_t pa =
	{
			  .pa_duty_cycle	= 0x04,
			  .hp_max			= 0x07,
			  .device_sel		= 0x00,
			  .pa_lut			= 0x01
	};
	if(sx126x_set_pa_cfg(NULL, &pa) 																			!= SX126X_STATUS_OK) 		{return SX126X_STATUS_ERROR;}
	if(sx126x_set_tx_params(NULL, 22, SX126X_RAMP_200_US) 														!= SX126X_STATUS_OK) 		{return SX126X_STATUS_ERROR;}

	if(sx126x_get_status(NULL, &radio_status) 																	!= SX126X_STATUS_OK)		{return SX126X_STATUS_ERROR;}

	return SX126X_STATUS_OK;																					// if there is no issue then return w/ STATUS OK
}

sx126x_status_t rfswitch_Tx(const void* context){
	HAL_GPIO_WritePin(SX1262_TXEN_GPIO_Port, SX1262_TXEN_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(SX1262_RXEN_GPIO_Port, SX1262_RXEN_Pin, GPIO_PIN_SET);

	if(sx126x_get_status(NULL, &radio_status) 																	!= SX126X_STATUS_OK)		{return SX126X_STATUS_ERROR;}

	return SX126X_STATUS_OK;																					// if there is no issue then return w/ STATUS OK
}

sx126x_status_t rfswitch_Rx(const void* context){
	HAL_GPIO_WritePin(SX1262_TXEN_GPIO_Port, SX1262_TXEN_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(SX1262_RXEN_GPIO_Port, SX1262_RXEN_Pin, GPIO_PIN_RESET);

	if(sx126x_get_status(NULL, &radio_status) 																	!= SX126X_STATUS_OK)		{return SX126X_STATUS_ERROR;}

	return SX126X_STATUS_OK;																					// if there is no issue then return w/ STATUS OK
}



