#include "board_sx126x_calls.h"
#include "main.h"

sx126x_chip_status_t radio_status;
sx126x_irq_mask_t irq_status = 0;								//set the interrupt flag to 0 at start
sx126x_status_t result = SX126X_STATUS_UNSUPPORTED_FEATURE;		//set to this so I can verify it changes later in code
sx126x_errors_mask_t errors;
uint32_t freq;

sx126x_status_t setup_22dBm_Tx(const void* context){
	sx126x_hal_reset(NULL);
	sx126x_clear_device_errors(NULL);
	//sx126x_set_dio2_as_rf_sw_ctrl(NULL, true);
	sx126x_set_standby(NULL, SX126X_STANDBY_CFG_RC);
	sx126x_set_dio3_as_tcxo_ctrl(NULL, SX126X_TCXO_CTRL_1_8V, sx126x_convert_timeout_in_ms_to_rtc_step(5));
	sx126x_set_reg_mode(NULL, SX126X_REG_MODE_DCDC);
	sx126x_set_pkt_type(NULL, SX126X_PKT_TYPE_LORA);
	sx126x_set_rf_freq(NULL, 915000000);
	sx126x_cal_img(NULL, 0xE1, 0xE9);

	sx126x_pa_cfg_params_t pa =
	{
			  .pa_duty_cycle	= 0x04,
			  .hp_max			= 0x07,
			  .device_sel		= 0x00,
			  .pa_lut			= 0x01
	};
	sx126x_set_pa_cfg(NULL, &pa);
	sx126x_set_tx_params(NULL, 22, SX126X_RAMP_200_US);

	sx126x_mod_params_lora_t mod =
	{
			  .sf   = SX126X_LORA_SF7,
			  .bw   = SX126X_LORA_BW_125,
			  .cr   = SX126X_LORA_CR_4_5,
			  .ldro = 0
	};
	sx126x_set_lora_mod_params(NULL, &mod);
	sx126x_pkt_params_lora_t pkt =
	{
			  .preamble_len_in_symb = 8,
	      	  .header_type          = SX126X_LORA_PKT_EXPLICIT,
	      	  .pld_len_in_bytes     = 5,
	      	  .crc_is_on            = true,
		  	  .invert_iq_is_on      = false
	};
	sx126x_set_lora_pkt_params(NULL, &pkt);
	uint8_t payload[] =
	{
			  'H',
			  'e',
			  'l',
			  'l',
			  'o'
	};
	sx126x_write_buffer(NULL, 0, payload, sizeof(payload));
	sx126x_set_dio_irq_params(NULL,
			  SX126X_IRQ_TX_DONE | SX126X_IRQ_TIMEOUT,
			  SX126X_IRQ_TX_DONE | SX126X_IRQ_TIMEOUT,	//DIO1)
			  SX126X_IRQ_NONE,							//DIO2
			  SX126X_IRQ_NONE							//DIO3
	);
	if(sx126x_get_status(NULL, &radio_status) != SX126X_STATUS_OK){
		return SX126X_STATUS_ERROR;
	}
	return SX126X_STATUS_OK;
}

sx126x_status_t rfswitch_Tx(const void* context){
	HAL_GPIO_WritePin(SX1262_TXEN_GPIO_Port, SX1262_TXEN_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(SX1262_RXEN_GPIO_Port, SX1262_RXEN_Pin, GPIO_PIN_SET);

	if(sx126x_get_status(NULL, &radio_status) != SX126X_STATUS_OK){
		return SX126X_STATUS_ERROR;
	}
	return SX126X_STATUS_OK;
}

sx126x_status_t transmit_pwr(const void* context){

	/** WIP
	 * don't have the code here yet. I'll work on that later once i have a better idea of what the output
	 * power looks like on the different TX modes supplied by the drivers.
	 *
	 * continuous wave (CW) looks promising, as it crams all the RF power into a small BW at 915MHz
	 *
	 * LR-FHSS is not actually supported inherently by the drivers (i.e. no discrete function to call); however,
	 * the drivers have all the functions that are needed to build a LR-FHSS in the call library
	 */

	if(sx126x_get_status(NULL, &radio_status) != SX126X_STATUS_OK){
		return SX126X_STATUS_ERROR;
	}
	return SX126X_STATUS_OK;
}


