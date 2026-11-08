#ifndef BOARD_SX126X_CALLS_H
#define BOARD_SX126X_CALLS_H

#include <stdbool.h>
#include <stdint.h>
#include "sx126x.h"
#include "sx126x_lr_fhss.h"

/* These functions must be called ONLY by RadioControl's owner thread. */
sx126x_status_t fhss_radio_configure(lr_fhss_v1_grid_t grid, lr_fhss_v1_bw_t bw);
sx126x_status_t fhss_radio_send(const uint8_t *payload, uint16_t length);
/* Read and clear SX1262 IRQ flags, handle LR-FHSS HOP/TX_DONE in radio task. */
sx126x_status_t fhss_radio_handle_irq(bool *tx_done);
sx126x_status_t fhss_radio_sleep(void);
void fhss_radio_rf_path_off(void);
bool fhss_radio_is_transmitting(void);
uint32_t fhss_radio_airtime_ms(uint16_t length);

#endif
