#include "board_sx126x_calls.h"
#include "main.h"

sx126x_chip_status_t radio_status;
sx126x_irq_mask_t irq_status = 0;								//set the interrupt flag to 0 at start
sx126x_errors_mask_t errors;
uint32_t freq;

static const uint8_t sync_word[4] = { 0x2C, 0x0F, 0x79, 0x95 };
static sx126x_lr_fhss_params_t fhss_params;
static sx126x_lr_fhss_state_t  fhss_state;     // type name: check the header
static uint32_t prng_state = 0;

volatile bool dio1_flag = false;
volatile bool tx_busy   = false;

#define CHECK(x) do { sx126x_status_t s_ = (x); if (s_ != SX126X_STATUS_OK) return s_; } while (0)

sx126x_status_t setup_fhss_22dBm_Tx(const void* context)
{
    sx126x_hal_reset(NULL);
    CHECK(sx126x_set_standby(NULL, SX126X_STANDBY_CFG_RC));
    CHECK(sx126x_clear_device_errors(NULL));
    CHECK(sx126x_set_dio3_as_tcxo_ctrl(NULL, SX126X_TCXO_CTRL_1_8V,
                                       sx126x_convert_timeout_in_ms_to_rtc_step(5)));
    CHECK(sx126x_set_reg_mode(NULL, SX126X_REG_MODE_DCDC));
    CHECK(sx126x_cal(NULL, SX126X_CAL_ALL));
    CHECK(sx126x_cal_img_in_mhz(NULL, 902, 928));

    fhss_params = (sx126x_lr_fhss_params_t){
        .center_freq_in_pll_steps = sx126x_convert_freq_in_hz_to_pll_step(915000000UL),
        .device_offset            = 0,
        .lr_fhss_params = {
            .sync_word       = sync_word,
            .modulation_type = LR_FHSS_V1_MODULATION_TYPE_GMSK_488,
            .cr              = LR_FHSS_V1_CR_1_3,
            .grid            = LR_FHSS_V1_GRID_25391_HZ,
            .bw              = LR_FHSS_V1_BW_1523438_HZ,
            .enable_hopping  = true,
            .header_count    = 3,
        },
    };

    CHECK(sx126x_lr_fhss_init(NULL, &fhss_params));

    sx126x_pa_cfg_params_t pa = { .pa_duty_cycle = 0x04, .hp_max = 0x07,
                                  .device_sel = 0x00, .pa_lut = 0x01 };
    CHECK(sx126x_set_pa_cfg(NULL, &pa));
    CHECK(sx126x_set_tx_params(NULL, 22, SX126X_RAMP_200_US));

    CHECK(sx126x_set_dio_irq_params(NULL,
            SX126X_IRQ_TX_DONE | SX126X_IRQ_LR_FHSS_HOP,
            SX126X_IRQ_TX_DONE | SX126X_IRQ_LR_FHSS_HOP,
            SX126X_IRQ_NONE, SX126X_IRQ_NONE));

    prng_seed(HAL_GetTick());				// seed used for psuedo random hopping algo

    return SX126X_STATUS_OK;
}

sx126x_status_t fhss_send(const uint8_t* payload, uint16_t len)
{
    uint16_t seq_count = sx126x_lr_fhss_get_hop_sequence_count(&fhss_params);
    if(seq_count == 0) return SX126X_STATUS_ERROR;								//seq_count cannot be zero; this would throw an error for the hop lookup in sx126x drivers
    uint16_t seq_id    = next_seq_id(seq_count);									//get next psuedo-random hop id
    uint32_t first_freq = 0;

    CHECK(sx126x_lr_fhss_build_frame(NULL, &fhss_params, &fhss_state, seq_id,
                                     payload, len, &first_freq));
    CHECK(sx126x_set_rf_freq_in_pll_steps(NULL, first_freq));   // tune to the first hop

    rfswitch_Tx(NULL);
    tx_busy = true;
    return sx126x_set_tx(NULL, 0);
}

void fhss_process(void)
{
    if (!dio1_flag) return;
    dio1_flag = false;

    sx126x_irq_mask_t irq;
    sx126x_get_and_clear_irq_status(NULL, &irq);

    if (irq & SX126X_IRQ_LR_FHSS_HOP)
        sx126x_lr_fhss_handle_hop(NULL, &fhss_params, &fhss_state);

    if (irq & SX126X_IRQ_TX_DONE) {
        sx126x_lr_fhss_handle_tx_done(NULL, &fhss_params, &fhss_state);
        rfswitch_Rx(NULL);          // or whatever idle state your TXEN/RXEN helper provides
        tx_busy = false;
    }
}

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
	HAL_GPIO_WritePin(SX1262_TXEN_GPIO_Port, SX1262_TXEN_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(SX1262_RXEN_GPIO_Port, SX1262_RXEN_Pin, GPIO_PIN_RESET);

	if(sx126x_get_status(NULL, &radio_status) 																	!= SX126X_STATUS_OK)		{return SX126X_STATUS_ERROR;}

	return SX126X_STATUS_OK;																					// if there is no issue then return w/ STATUS OK
}

sx126x_status_t rfswitch_Rx(const void* context){
	HAL_GPIO_WritePin(SX1262_TXEN_GPIO_Port, SX1262_TXEN_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(SX1262_RXEN_GPIO_Port, SX1262_RXEN_Pin, GPIO_PIN_SET);

	if(sx126x_get_status(NULL, &radio_status) 																	!= SX126X_STATUS_OK)		{return SX126X_STATUS_ERROR;}

	return SX126X_STATUS_OK;																					// if there is no issue then return w/ STATUS OK
}



// rng func --------------------------------------------------------------------------------------------
static void prng_seed(uint32_t extra)
{
    uint32_t s = HAL_GetUIDw0() ^ (HAL_GetUIDw1() * 2654435761u) ^ HAL_GetUIDw2() ^ extra;
    prng_state = s ? s : 0x1F2E3D4Cu;          // xorshift must never be 0
}

static uint32_t prng_next(void)                // xorshift32
{
    uint32_t x = prng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return prng_state = x;
}

static void deck_shuffle(uint16_t count)
{
    deck_count = (count > MAX_SEQ) ? MAX_SEQ : count;
    for (uint16_t i = 0; i < deck_count; i++) deck[i] = i;
    for (uint16_t i = deck_count - 1; i > 0; i--) {          // Fisher-Yates
        uint16_t j = prng_next() % (i + 1);
        uint16_t t = deck[i]; deck[i] = deck[j]; deck[j] = t;
    }
    deck_pos = 0;
}

static uint16_t next_seq_id(uint16_t count)
{
    if (deck_pos >= deck_count || deck_count == 0) deck_shuffle(count);
    return deck[deck_pos++];
}
