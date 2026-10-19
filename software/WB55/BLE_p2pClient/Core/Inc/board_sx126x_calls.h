#ifndef BOARD_SX126X_CALLS_H
#define BOARD_SX126X_CALLS_H
#endif
#include "sx126x.h"
#include "sx126x_hal.h"
#include "sx126x_driver_version.h"

/**
 * @brief setup the sx1262 for 22dBm transmit
 */
sx126x_status_t setup_22dBm_Tx(const void* context);

/**
 * @brief configure RF switch for TX
 */
sx126x_status_t rfswitch_Tx(const void* context);

/**
 * @brief send power to receiving board
 */
sx126x_status_t transmit_pwr(const void* context);
