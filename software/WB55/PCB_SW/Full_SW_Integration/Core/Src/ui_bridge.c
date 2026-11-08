#include "ui_bridge.h"
#include "cmsis_os2.h"
#include <string.h>

/* UI_Queue is created before MX_APPE_Init() in main.c. */
extern osMessageQueueId_t UI_QueueHandle;

void UI_PostBLEText(const uint8_t *data, uint16_t length)
{
    if ((data == NULL) || (UI_QueueHandle == NULL))
        return;

    ui_event_t msg = {0};
    msg.type = UI_EVENT_BLE_TEXT;

    if (length > UI_BLE_TEXT_MAX)
        length = UI_BLE_TEXT_MAX;

    for (uint16_t i = 0; i < length; i++)
    {
        uint8_t c = data[i];
        msg.data.text[i] = (c >= 32U && c <= 126U) ? (char)c : '?';
    }
    msg.data.text[length] = '\0';

    /* Do not block ST's HCI event-processing task. */
    (void)osMessageQueuePut(UI_QueueHandle, &msg, 0, 0);
}

bool UI_PostTLMReading(const ui_tlm_reading_t *reading)
{
    if ((reading == NULL) || (UI_QueueHandle == NULL))
        return false;

    ui_event_t msg = {0};
    msg.type = UI_EVENT_TLM_READING;
    msg.data.tlm = *reading;

    /* Only the UI task writes the ten-element history ring. */
    return osMessageQueuePut(UI_QueueHandle, &msg, 0, 0) == osOK;
}
