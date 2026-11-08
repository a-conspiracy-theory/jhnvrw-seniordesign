#include "radio_control.h"
#include "board_sx126x_calls.h"
#include "ui_bridge.h"
#include "cmsis_os2.h"
#include "main.h"

/* Radio-task wakeups. Command requests and SX1262 DIO1 interrupts share one
 * CMSIS thread-flags wait, so no one-tick polling is needed for LR-FHSS hops. */
#define RADIO_EVENT_COMMAND     (1UL << 0)
#define RADIO_EVENT_DIO1        (1UL << 1)
#define RADIO_EVENT_MASK        (RADIO_EVENT_COMMAND | RADIO_EVENT_DIO1)
#define RADIO_IDLE_BETWEEN_PACKETS_MS 50U
#define RADIO_IRQ_DRAIN_LIMIT   8U

static osThreadId_t radio_thread;
static osMessageQueueId_t radio_queue;
/* Accessed in the EXTI ISR and radio task. Armed just before starting TX. */
static volatile bool radio_dio1_armed;
/* Watch these in STM32CubeIDE to validate EXTI wakeups and task servicing. */
static volatile uint32_t radio_dio1_edge_count;
static volatile uint32_t radio_irq_service_count;
static volatile uint32_t radio_tx_watchdog_count;

static const osThreadAttr_t radio_attr = {
    .name = "SX1262_FHSS",
    .stack_size = 2048U,
    .priority = osPriorityAboveNormal
};

/* Demo payload, replace with the application's transmitted data. */
static const uint8_t demo_payload[] = {'F','H','S','S','_','V','6'};

static uint32_t radio_ms_to_ticks(uint32_t ms)
{
    const uint32_t hz = osKernelGetTickFreq();
    uint64_t ticks = ((uint64_t)ms * hz + 999U) / 1000U;
    return (uint32_t)((ticks == 0U) ? 1U :
                      (ticks > 0x7FFFFFFFU ? 0x7FFFFFFFU : ticks));
}

static uint32_t radio_until(uint32_t now, uint32_t deadline)
{
    return ((int32_t)(deadline - now) <= 0) ? 0U : (deadline - now);
}

static void radio_report(radio_state_t state, sx126x_status_t status)
{
    ui_event_t event = {0};
    event.type = UI_EVENT_RADIO_STATUS;
    event.data.radio.state = state;
    event.data.radio.error = (uint8_t)status;
    extern osMessageQueueId_t UI_QueueHandle;
    if (UI_QueueHandle != NULL)
        (void)osMessageQueuePut(UI_QueueHandle, &event, 0U, 0U);
}

static void radio_stop(bool *enabled, sx126x_status_t status)
{
    radio_dio1_armed = false;
    sx126x_status_t sleep_status = SX126X_STATUS_OK;
    if (*enabled)
        sleep_status = fhss_radio_sleep();
    else
        fhss_radio_rf_path_off();
    *enabled = false;
    if (status == SX126X_STATUS_OK) status = sleep_status;
    radio_report(status == SX126X_STATUS_OK ? RADIO_STATE_OFF : RADIO_STATE_ERROR,
                 status);
}

/* Runs in GPIO EXTI0 interrupt context. No SX1262 SPI/HAL delays here. */
void RadioControl_DIO1_IRQHandler(void)
{
    if (radio_dio1_armed && radio_thread != NULL)
    {
        radio_dio1_edge_count++;
        (void)osThreadFlagsSet(radio_thread, RADIO_EVENT_DIO1);
    }
}

static void radio_task(void *argument)
{
    (void)argument;
    bool enabled = false;
    uint32_t next_send = 0U;
    uint32_t tx_deadline = 0U;
    radio_command_t cmd;

    radio_dio1_armed = false;
    fhss_radio_rf_path_off();

    for (;;)
    {
        /* Sleep until UI requests power/config changes, DIO1 asserts, or
         * the next transmission / TX watchdog deadline is reached. */
        uint32_t now = osKernelGetTickCount();
        uint32_t wait_ticks = osWaitForever;
        if (enabled)
            wait_ticks = radio_until(now, fhss_radio_is_transmitting()
                                            ? tx_deadline : next_send);

        uint32_t events = 0U;
        if (wait_ticks != 0U)
            events = osThreadFlagsWait(RADIO_EVENT_MASK, osFlagsWaitAny,
                                        wait_ticks);

        /* Prioritize OFF commands over servicing or starting any TX packet. */
        while (osMessageQueueGet(radio_queue, &cmd, NULL, 0U) == osOK)
        {
            if (!cmd.enable)
            {
                radio_stop(&enabled, SX126X_STATUS_OK);
                continue;
            }

            radio_dio1_armed = false;
            if (enabled) (void)fhss_radio_sleep();
            enabled = false;
            sx126x_status_t s = fhss_radio_configure(cmd.grid, cmd.bw);
            if (s != SX126X_STATUS_OK)
            {
                (void)fhss_radio_sleep();
                fhss_radio_rf_path_off();
                radio_report(RADIO_STATE_ERROR, s);
                continue;
            }
            enabled = true;
            next_send = osKernelGetTickCount();
            radio_report(RADIO_STATE_ON, SX126X_STATUS_OK);
        }

        if (!enabled) continue;

        /* The SX1262 raises DIO1 for each LR-FHSS HOP request and TX_DONE.
         * Drain latched IRQs in this task, never in the EXTI callback.
         * A pin-level check also catches an edge that arrived while
         * processing a previous notification. */
        if (fhss_radio_is_transmitting() &&
            (((events & osFlagsError) == 0U &&
              (events & RADIO_EVENT_DIO1) != 0U) ||
             HAL_GPIO_ReadPin(SX1262_DIO1_GPIO_Port,
                              SX1262_DIO1_Pin) == GPIO_PIN_SET))
        {
            bool done = false;
            sx126x_status_t s = SX126X_STATUS_OK;
            uint32_t i;
            for (i = 0U; i < RADIO_IRQ_DRAIN_LIMIT; ++i)
            {
                radio_irq_service_count++;
                s = fhss_radio_handle_irq(&done);
                if (s != SX126X_STATUS_OK || done ||
                    HAL_GPIO_ReadPin(SX1262_DIO1_GPIO_Port,
                                     SX1262_DIO1_Pin) == GPIO_PIN_RESET)
                    break;
            }
            if (s == SX126X_STATUS_OK && !done &&
                i == RADIO_IRQ_DRAIN_LIMIT)
                s = SX126X_STATUS_ERROR; /* Persistent asserted DIO1 */

            if (s != SX126X_STATUS_OK)
            {
                radio_stop(&enabled, s);
                continue;
            }
            if (done)
            {
                radio_dio1_armed = false;
                next_send = osKernelGetTickCount() +
                            radio_ms_to_ticks(RADIO_IDLE_BETWEEN_PACKETS_MS);
            }
        }

        now = osKernelGetTickCount();
        if (fhss_radio_is_transmitting())
        {
            if ((int32_t)(now - tx_deadline) >= 0)
            {
                radio_tx_watchdog_count++;
                radio_stop(&enabled, SX126X_STATUS_ERROR);
            }
            continue;
        }

        if ((int32_t)(now - next_send) >= 0)
        {
            /* Arm BEFORE sx126x_set_tx(), which can raise DIO1 promptly. */
            radio_dio1_armed = true;
            sx126x_status_t s = fhss_radio_send(demo_payload,
                                               sizeof(demo_payload));
            if (s != SX126X_STATUS_OK)
            {
                radio_stop(&enabled, s);
            }
            else
            {
                uint32_t toa_ms = fhss_radio_airtime_ms(sizeof(demo_payload));
                tx_deadline = osKernelGetTickCount() +
                              radio_ms_to_ticks(toa_ms * 2U + 1000U);
            }
        }
    }
}

bool RadioControl_Create(void)
{
    radio_dio1_armed = false;
    radio_queue = osMessageQueueNew(4U, sizeof(radio_command_t), NULL);
    if (radio_queue == NULL) return false;
    radio_thread = osThreadNew(radio_task, NULL, &radio_attr);
    return radio_thread != NULL;
}

bool RadioControl_Request(const radio_command_t *cmd)
{
    if (radio_queue == NULL || radio_thread == NULL || cmd == NULL)
        return false;
    if (osMessageQueuePut(radio_queue, cmd, 0U, 0U) != osOK)
        return false;
    /* Wakes the task even when Power is OFF and it is waiting indefinitely. */
    (void)osThreadFlagsSet(radio_thread, RADIO_EVENT_COMMAND);
    return true;
}
