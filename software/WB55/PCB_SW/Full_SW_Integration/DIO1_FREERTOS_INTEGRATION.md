# STM32WB55 + SX1262 DIO1 + FreeRTOS (V7)

This is the **uploaded Phil_Vid_Test_V7_FHSS project**, changed to handle the SX1262 DIO1 interrupt on **PA0 / EXTI0** rather than polling radio IRQ status every millisecond.

## Important correction to the supplied V7 configuration

The uploaded `.ioc` already labels **PA0 = SX1262_DIO1** with signal `GPXTI0`, but it **did not** set `GPIO_MODE_IT_RISING` or enable `NVIC.EXTI0_IRQn`. Consequently, its generated `MX_GPIO_Init` and interrupt source files did not configure or service that interrupt.

This update completes the `.ioc` settings and synchronizes the generated files:

- `Phil_Vid_Test.ioc`: PA0 rising-edge EXTI, no internal pull, EXTI0 IRQ at priority **5**.
- `Core/Inc/main.h`: `SX1262_DIO1_Pin`, `_GPIO_Port` and `_EXTI_IRQn`.
- `Core/Src/main.c`: PA0 GPIO configuration, NVIC enable; protected `HAL_GPIO_EXTI_Callback` routes DIO1 to radio task notification (button handler unchanged).
- `Core/Inc/stm32wbxx_it.h` and `Core/Src/stm32wbxx_it.c`: EXTI0 declaration and handler calling `HAL_GPIO_EXTI_IRQHandler(SX1262_DIO1_Pin)`.

**FreeRTOS interrupt priority:** `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` is 5 in this project, so EXTI0 uses preemption priority 5 (not 0). CMSIS-RTOS2 `osThreadFlagsSet` is allowed in interrupt context at this priority.

## Radio architecture

1. Power ON from OLED → `RadioControl_Request` enqueues settings and sets `RADIO_EVENT_COMMAND`.
2. `SX1262_FHSS` task configures SX1262 over SPI, then starts transmission.
3. On an LR-FHSS HOP request or TX_DONE, the SX1262 asserts DIO1, raising **PA0/EXTI0**.
4. `EXTI0_IRQHandler` → `HAL_GPIO_EXTI_IRQHandler` → `HAL_GPIO_EXTI_Callback` → `RadioControl_DIO1_IRQHandler`.
5. **ISR only sets** `RADIO_EVENT_DIO1` via `osThreadFlagsSet`; no SPI and no delays in the ISR.
6. Radio task wakes from `osThreadFlagsWait`, calls `fhss_radio_handle_irq`, which reads/clears IRQ status and executes `sx126x_lr_fhss_handle_hop` or `sx126x_lr_fhss_handle_tx_done`.
7. TX_DONE schedules the next packet after 50 ms. Power OFF wakes the same task via the COMMAND flag, and places the SX1262 in cold sleep.

**DIO1 represents a hop request and TX completion, not necessarily the end of the entire hopping sequence.** The radio task must handle *each* LR-FHSS hop request promptly. TX_DONE is the completion indication.

While transmitting, the radio task sleeps waiting for **DIO1/command events**, with a bounded watchdog deadline if TX_DONE never arrives. A brief bounded drain of a still-asserted DIO1 line handles latched IRQs. No per-tick status polling remains.

The BLE scanner, UI tasks, queues, history, and UI RF settings are retained. No radio-driver/SPI code runs from GPIO interrupt context.

## Debugging on board

With a safe RF setup (properly terminated antenna path or RF load/attenuation appropriate to the module and applicable regulations):

1. Build/flash this V7 project, enter Settings and choose a valid grid/bandwidth, then select Power.
2. Set a breakpoint in `HAL_GPIO_EXTI_Callback`, filter for `GPIO_Pin == SX1262_DIO1_Pin`. Check that EXTI0 is enabled at priority 5.
3. Watch the **static volatile** variables in `Core/Src/radio_control.c`:
   - `radio_dio1_edge_count` — increments for DIO1 edges when transmit IRQs are armed.
   - `radio_irq_service_count` — increments when the radio task handles IRQ status.
   - `radio_tx_watchdog_count` — increments if TX_DONE is missed past the watchdog deadline (normally zero).
4. Break in `fhss_radio_handle_irq()`, verify `irq` includes `SX126X_IRQ_LR_FHSS_HOP` repeatedly and eventually `SX126X_IRQ_TX_DONE`.
5. During continuous transmissions, confirm the OLED Power state remains `TX ON`, and selecting Power again stops transmission and reports OFF.

If there are **no DIO1 edges**, verify the physical DIO1-to-PA0 connection and SX1262 IRQ routing in `fhss_radio_configure()`, and inspect the NVIC/EXTI GPIO settings. If edges occur but no hop is serviced, verify `osThreadFlagsSet` and the radio task priorities. If watchdog failures occur, check SPI BUSY timing, task latency, DIO1 electrical level and IRQ masks.

## Regeneration and validation

The `.ioc` explicitly contains the new PA0/EXTI0 configuration. `main.h`, `main.c`'s generated GPIO section, and the generated `stm32wbxx_it.*` handler have been synchronized to it; CubeMX should reproduce these sections on regeneration. The custom ISR-to-task hook is inside `main.c`'s `USER CODE BEGIN 4`, and radio worker/driver files are separate application code. Always back up or commit the project before regenerating.

Source-level host GCC syntax checks passed for the modified C sources. **An ARM cross-link, actual CubeMX regeneration, and physical RF/interrupt timing tests were not performed.** Under CPU contention, even interrupt-driven hopping can miss RF deadlines; measure timing under concurrent BLE/OLED load before relying on the design.
