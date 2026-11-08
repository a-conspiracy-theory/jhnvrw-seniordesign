#ifndef UI_BRIDGE_H
#define UI_BRIDGE_H

#include <stdbool.h>
#include <stdint.h>
#include "radio_control.h"

#define UI_BLE_TEXT_MAX 16

/* Shared by main.c (UI task) and the BLE callbacks. */
typedef enum
{
    UI_EVENT_BUTTON = 0,
    UI_EVENT_BLE_TEXT = 1,
    UI_EVENT_TLM_READING = 2,
    UI_EVENT_RADIO_STATUS = 3
} ui_event_type_t;

/* Timestamp is elapsed seconds on the STM32, NOT time of day. */
typedef struct
{
    uint16_t humidity_raw;
    uint16_t temperature_raw;
    uint32_t received_seconds;
    uint32_t adv_count;
} ui_tlm_reading_t;

typedef struct
{
    ui_event_type_t type;
    union
    {
        int button;
        char text[UI_BLE_TEXT_MAX + 1];
        ui_tlm_reading_t tlm;
        radio_report_t radio;
    } data;
} ui_event_t;

void UI_PostBLEText(const uint8_t *data, uint16_t length);
bool UI_PostTLMReading(const ui_tlm_reading_t *reading);

#endif /* UI_BRIDGE_H */
