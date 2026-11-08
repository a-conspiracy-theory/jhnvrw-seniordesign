/* LR-FHSS setup and hopping adapted from FHSS_DEV.
 * SX1262 SPI and frequency-hop state are owned by the FreeRTOS radio task.
 * The PA0/DIO1 EXTI ISR only wakes that task; IRQ status is read/cleared here.
 */
#include "board_sx126x_calls.h"
#include "sx126x_hal.h"
#include "main.h"

#define FHSS_CENTER_HZ 915000000UL
#define FHSS_TX_POWER_DBM 22
#define FHSS_SEQUENCE_COUNT_MAX 512U
#define CHECK(call) do { sx126x_status_t _s = (call); if (_s != SX126X_STATUS_OK) return _s; } while (0)

static const uint8_t sync_word[4] = {0x2C, 0x0F, 0x79, 0x95};
static sx126x_lr_fhss_params_t params;
static sx126x_lr_fhss_state_t state;
static bool sending;
static uint16_t deck[FHSS_SEQUENCE_COUNT_MAX];
static uint16_t deck_size, deck_pos;
static uint32_t prng_state;

static uint32_t random_next(void)
{
    uint32_t x = prng_state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return prng_state = x;
}
static void shuffle(uint16_t count)
{
    deck_size = count;
    for (uint16_t i = 0; i < count; ++i) deck[i] = i;
    for (uint16_t i = count - 1; i > 0; --i) {
        uint16_t j = (uint16_t)(random_next() % (i + 1U));
        uint16_t tmp = deck[i]; deck[i] = deck[j]; deck[j] = tmp;
    }
    deck_pos = 0;
}
static uint16_t sequence_next(uint16_t count)
{
    if (deck_pos >= deck_size || deck_size != count) shuffle(count);
    return deck[deck_pos++];
}

void fhss_radio_rf_path_off(void)
{
    HAL_GPIO_WritePin(SX126X_TXEN_GPIO_Port, SX126X_TXEN_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SX1262_RXEN_GPIO_Port, SX1262_RXEN_Pin, GPIO_PIN_RESET);
}
static void rf_path_tx(void)
{
    HAL_GPIO_WritePin(SX1262_RXEN_GPIO_Port, SX1262_RXEN_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SX126X_TXEN_GPIO_Port, SX126X_TXEN_Pin, GPIO_PIN_SET);
}

bool fhss_radio_is_transmitting(void) { return sending; }
uint32_t fhss_radio_airtime_ms(uint16_t length)
{
    return sx126x_lr_fhss_get_time_on_air_in_ms(&params, length);
}

sx126x_status_t fhss_radio_configure(lr_fhss_v1_grid_t grid, lr_fhss_v1_bw_t bw)
{
    /* The Semtech driver rejects the six narrowest BW values on 25.391k grid. */
    if ((grid != LR_FHSS_V1_GRID_25391_HZ && grid != LR_FHSS_V1_GRID_3906_HZ) ||
        bw > LR_FHSS_V1_BW_1574219_HZ ||
        (grid == LR_FHSS_V1_GRID_25391_HZ && bw < LR_FHSS_V1_BW_722656_HZ))
        return SX126X_STATUS_UNKNOWN_VALUE;

    sending = false;
    deck_size = 0; deck_pos = 0;
    fhss_radio_rf_path_off();
    if (sx126x_hal_reset(NULL) != SX126X_HAL_STATUS_OK) return SX126X_STATUS_ERROR;
    CHECK(sx126x_set_standby(NULL, SX126X_STANDBY_CFG_RC));
    CHECK(sx126x_clear_device_errors(NULL));
    CHECK(sx126x_set_dio3_as_tcxo_ctrl(NULL, SX126X_TCXO_CTRL_1_8V,
                                       sx126x_convert_timeout_in_ms_to_rtc_step(5)));
    CHECK(sx126x_set_reg_mode(NULL, SX126X_REG_MODE_DCDC));
    CHECK(sx126x_cal(NULL, SX126X_CAL_ALL));
    CHECK(sx126x_cal_img_in_mhz(NULL, 902, 928));

    params = (sx126x_lr_fhss_params_t) {
        .center_freq_in_pll_steps = sx126x_convert_freq_in_hz_to_pll_step(FHSS_CENTER_HZ),
        .device_offset = 0,
        .lr_fhss_params = {
            .sync_word = sync_word,
            .modulation_type = LR_FHSS_V1_MODULATION_TYPE_GMSK_488,
            .cr = LR_FHSS_V1_CR_1_3,
            .grid = grid,
            .bw = bw,
            .enable_hopping = true,
            .header_count = 3
        }
    };
    CHECK(sx126x_lr_fhss_init(NULL, &params));
    sx126x_pa_cfg_params_t pa = {
        .pa_duty_cycle = 0x04, .hp_max = 0x07, .device_sel = 0x00, .pa_lut = 0x01
    };
    CHECK(sx126x_set_pa_cfg(NULL, &pa));
    CHECK(sx126x_set_tx_params(NULL, FHSS_TX_POWER_DBM, SX126X_RAMP_200_US));
    CHECK(sx126x_set_dio_irq_params(NULL,
        SX126X_IRQ_TX_DONE | SX126X_IRQ_LR_FHSS_HOP,
        SX126X_IRQ_TX_DONE | SX126X_IRQ_LR_FHSS_HOP,
        SX126X_IRQ_NONE, SX126X_IRQ_NONE));
    CHECK(sx126x_clear_irq_status(NULL, SX126X_IRQ_ALL));

    prng_state = HAL_GetUIDw0() ^ (HAL_GetUIDw1() * 2654435761UL) ^ HAL_GetUIDw2() ^ HAL_GetTick();
    if (!prng_state) prng_state = 0x1F2E3D4CU;
    return SX126X_STATUS_OK;
}

sx126x_status_t fhss_radio_send(const uint8_t *payload, uint16_t length)
{
    if (sending || !payload || !length) return SX126X_STATUS_ERROR;
    uint16_t count = sx126x_lr_fhss_get_hop_sequence_count(&params);
    if (!count || count > FHSS_SEQUENCE_COUNT_MAX) return SX126X_STATUS_ERROR;
    uint32_t first_freq = 0;
    CHECK(sx126x_lr_fhss_build_frame(NULL, &params, &state,
                                      sequence_next(count), payload, length, &first_freq));
    CHECK(sx126x_set_rf_freq_in_pll_steps(NULL, first_freq));
    rf_path_tx();
    sx126x_status_t result = sx126x_set_tx(NULL, 0);
    if (result != SX126X_STATUS_OK) fhss_radio_rf_path_off();
    else sending = true;
    return result;
}

sx126x_status_t fhss_radio_handle_irq(bool *tx_done)
{
    if (tx_done) *tx_done = false;
    if (!sending) return SX126X_STATUS_OK;
    sx126x_irq_mask_t irq = SX126X_IRQ_NONE;
    CHECK(sx126x_get_and_clear_irq_status(NULL, &irq));
    if (irq & SX126X_IRQ_LR_FHSS_HOP)
        CHECK(sx126x_lr_fhss_handle_hop(NULL, &params, &state));
    if (irq & SX126X_IRQ_TX_DONE) {
        CHECK(sx126x_lr_fhss_handle_tx_done(NULL, &params, &state));
        fhss_radio_rf_path_off();
        sending = false;
        if (tx_done) *tx_done = true;
    }
    return SX126X_STATUS_OK;
}

sx126x_status_t fhss_radio_sleep(void)
{
    sending = false;
    /* Abort TX, disable RF switch, and enter SX1262 cold sleep.
     * This is NOT a physical power supply disconnection. */
    sx126x_status_t result = sx126x_set_standby(NULL, SX126X_STANDBY_CFG_RC);
    fhss_radio_rf_path_off();
    if (result != SX126X_STATUS_OK) return result;
    return sx126x_set_sleep(NULL, SX126X_SLEEP_CFG_COLD_START);
}
