#ifndef BOARD_SX126X_CALLS_H
#define BOARD_SX126X_CALLS_H
#endif
#include "sx126x.h"
#include "sx126x_hal.h"
#include "sx126x_driver_version.h"
#include "sx126x_lr_fhss.h"

extern volatile bool dio1_flag;
extern volatile bool tx_busy;

// rng privates
#define MAX_SEQ 512
static uint16_t deck[MAX_SEQ];
static uint16_t deck_count = 0, deck_pos = 0;
//


/**
 * @brief setup the sx1262 for 22dBm transmit
 */
sx126x_status_t setup_22dBm_Tx(const void* context);

/**
 * @brief configure RF switch for TX
 */
sx126x_status_t rfswitch_Tx(const void* context);

/**
 * @brief configure RF switch for RX
 */
sx126x_status_t rfswitch_Rx(const void* context);

/**
 * @brief send power to receiving board
 */

/**
 * @ brief setup FHSS w/ 50 channels at 22dBm
 */
sx126x_status_t setup_fhss_22dBm_Tx(const void* context);

sx126x_status_t fhss_send(const uint8_t* payload, uint16_t len);

void fhss_process(void);


// rng func
static void prng_seed(uint32_t extra);

static uint32_t prng_next(void);

static void deck_shuffle(uint16_t count);

static uint16_t next_seq_id(uint16_t count);
// rng func end
