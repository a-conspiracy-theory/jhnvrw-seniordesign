# SHT41 engineering-unit display update

Updated `Drivers/ssd1306/src/ui.c` only. All FreeRTOS, BLE scanner, queue, and `.ioc` settings are preserved from `Phil_Vid_Test_CubeMX_Safe_TLM_History.zip`.

* Main menu right half shows latest temperature (Celsius) and relative humidity (%RH) from the newest Eddystone-TLM sample. Until the first reading, shows placeholders.
* History shows decoded °C and %RH values instead of raw 16-bit hex sensor values, keeping timestamp and the newest-first 10-reading ring buffer.
* BLE Settings shows latest values in engineering units as well.
* Sensor conversion is performed in UI code, on the UI task, and **does not change** the BLE frame or queue layout.
* Temperature: -45 + 175 * raw / 65535 (Celsius). This **corrects** the example's 315 factor, which belongs to Fahrenheit with a -49 offset.
* RH: -6 + 125 * raw / 65535, clamped to 0..100%.
* Display formatting uses integer tenths instead of `%f`, so it does not require linking the nano printf float implementation.

## Re-generating in CubeMX
No `.ioc` changes are required for this update. The custom OLED UI is in `Drivers/ssd1306/src/ui.c`; this project keeps that file under the project-controlled custom driver directory. Retain `Keep User Code` and your full project backup. Your CubeMX code-generator settings and the BLE-protected user code are unchanged.

## Verification
Host GCC syntax compilation of `ui.c` with lightweight OLED stubs passed. Not built with STM32CubeIDE and not tested on target hardware.
