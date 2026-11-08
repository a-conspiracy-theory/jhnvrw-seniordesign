# TLM scan history (FreeRTOS)

This project starts from `Phil_Vid_Test_TLM_FreeRTOS.zip` and keeps the original scan task and UI/Button FreeRTOS tasks.

## Behavior

- The BLE callback recognizes the same custom Eddystone-TLM FEAA/0x20 packet as before.
- At most one accepted reading per second is queued to the UI task. The history records accepted scan samples, not every individual on-air packet.
- The UI task stores the latest **10** readings in a circular buffer, newest first; old entries are overwritten.
- `History` displays **two** readings at once because a 128 x 64 display using 7 x 10 glyphs cannot show 10 two-line records at once.
- UP and DOWN move the list one record at a time. RIGHT jumps to the newest. LEFT or MID returns to the main menu. Entering History resets the view to the newest.
- Each record shows an index (`01` = newest), `HH:MM:SS` elapsed time since STM32 RTOS startup, and the same **raw hexadecimal** SHT41 humidity/temperature values used by the trial.
- Timestamp is **uptime**, not date/time. For calendar time, initialize the STM32 RTC to a known time and capture it in the reading event.
- History lives in RAM; it is lost on restart or reset.

## Files changed

- `Core/Inc/ui_bridge.h`: shared event type and TLM record.
- `Core/Src/ui_bridge.c`: non-blocking queue submission for a reading.
- `Core/Src/main.c`: UI task consumes readings; UI thread stack grows from 512 B to 1024 B.
- `Drivers/ssd1306/inc/ui.h`: history API.
- `Drivers/ssd1306/src/ui.c`: history ring, elapsed timestamps, rendering and navigation.
- `STM32_WPAN/App/app_ble.c`: queue one TLM reading per second and timestamp it using CMSIS-RTOS tick count.
- `Phil_Vid_Test.ioc`: UI task stack matches main.c (256 words).

## Validation

Host-compiled UI test passed for empty history, 10-entry circular rollover, newest-first ordering, and button navigation. Source syntax checks passed for ui.c and ui_bridge.c. The complete STM32 project **has not been built with the ARM toolchain or tested on hardware** in this environment.

## Diagnostics

Open History and verify new entries arrive. The BLE Settings screen still displays the latest raw pair, e.g. `H:7F00 T:6789`. If no entries arrive, set breakpoints in `TLM_ParseServiceData()`, `UI_PostTLMReading()`, `UITask()`'s `UI_EVENT_TLM_READING` branch, and `UI_AddTLMReading()`.
