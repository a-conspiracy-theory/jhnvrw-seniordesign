#ifndef RADIO_CONTROL_H
#define RADIO_CONTROL_H

#include <stdbool.h>
#include <stdint.h>
#include "lr_fhss_v1_base_types.h"

typedef struct {
    bool enable;
    lr_fhss_v1_grid_t grid;
    lr_fhss_v1_bw_t bw;
} radio_command_t;

typedef enum {
    RADIO_STATE_OFF = 0,
    RADIO_STATE_ON,
    RADIO_STATE_ERROR
} radio_state_t;

typedef struct {
    radio_state_t state;
    uint8_t error;  /* SX126X status, zero on success */
} radio_report_t;

/* Call before osKernelStart(), after osKernelInitialize(). */
bool RadioControl_Create(void);
/* UI task queues a command; this never accesses SPI. */
bool RadioControl_Request(const radio_command_t *cmd);
/* Called exclusively from the HAL_GPIO_EXTI_Callback for SX1262 DIO1.
 * No SPI, blocking calls, or other driver code may run here. */
void RadioControl_DIO1_IRQHandler(void);

#endif /* RADIO_CONTROL_H */
