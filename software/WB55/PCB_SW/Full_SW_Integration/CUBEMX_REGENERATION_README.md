# CubeMX-regeneration-safe BLE/TLM History integration

This project is based on the working `Phil_Vid_Test_TLM_History` STM32WB55CCU6 project. It retains the FreeRTOS UI and button tasks, nonblocking scan-to-UI queue, 10-record OLED History ring, and uptime timestamps.

## Changes for regeneration safety

1. **Keep `STM32_WPAN.BLE_APPLICATION_TYPE=BLE_SERVER_PROFILE` in `Phil_Vid_Test.ioc`** to preserve the project's generated server/service configuration. ST recommends adding central behavior through user-code sections if retaining the CubeMX server setup. Do **not** change the profile to Router unless you intend to rebuild the BLE service configuration.
2. **`ble_conf.h` is untouched relative to CubeMX output.** Its generated `BLE_CFG_CENTRAL` setting is 0; the central GAP role is enabled through `role |= GAP_CENTRAL_ROLE;` inside `/* USER CODE BEGIN Role_Mngt*/` in `app_ble.c`.
3. **`app_ble.c` contains all custom BLE logic within protected `USER CODE` sections.** The generated `Adv_Request(APP_BLE_FAST_ADV);` calls are restored, not edited or commented out:
   - `TLM_SCAN_ONLY_MODE` (defined in `USER CODE BEGIN PD`, currently 1) selects scan-only operation.
   - `USER CODE BEGIN APP_BLE_Init_3` creates `TLM_ScanThread` **after** HCI/GAP/GATT and service initialization and then returns before CubeMX's advertising setup and first advertising call. The generated advertising timer is not created in scan-only mode (it is not needed when no connections are initiated).
   - `USER CODE BEGIN EVT_DISCONN_COMPLETE_1` has a scan-only `break;` that bypasses CubeMX's generated re-advertising call in the unlikely case of a disconnection event.
   - TLM parsing, scan restarts, and UI-queue submission are otherwise unchanged from the prior History project.
4. **`Phil_Vid_Test.ioc` has `KeepUserCode=true`, `BackupPrevious=true`, and `DeletePrevious=false`.** FreeRTOS heap remains `24*1024`, and the UI task stack remains 256 words (1,024 bytes).
5. **`ui.c`, `ui.h`, `ui_bridge.c`, and `ui_bridge.h`** are independent application files and are not altered by these changes. Their queue and History behavior is preserved.

## How to regenerate safely

1. Back up/commit the project. Open **`Phil_Vid_Test.ioc` from the project itself** in the STM32CubeIDE integrated CubeMX editor.
2. Confirm **Project Manager > Code Generator > Keep User Code** remains enabled, along with backup of previously generated files.
3. Keep the **BLE Server Profile** unless you plan to rebuild the server configuration. The `BLE_CFG_CENTRAL=0` in the regenerated `ble_conf.h` is expected, because the central role is added by a protected `Role_Mngt` hook.
4. Regenerate and verify that `app_ble.c` still contains `TLM_SCAN_ONLY_MODE`, `APP_BLE_Init_3` returning after scan-task creation, and `role |= GAP_CENTRAL_ROLE;` in `Role_Mngt`.
5. Rebuild in STM32CubeIDE and flash the STM32WB55CCU6. In scan-only mode it intentionally **does not advertise to ST BLE Toolbox**, but receives the WB05 Eddystone-TLM payloads and feeds the OLED History.
6. If CubeMX regenerates or changes the names of the BLE user-code markers in a **future middleware version**, check/reapply the hooks before running; no regeneration strategy guarantees compatibility across major template changes.

## Switching between scan-only and generated advertising

`STM32_WPAN/App/app_ble.c`:

```c
/* USER CODE BEGIN PD */
#define TLM_SCAN_ONLY_MODE 1
/* USER CODE END PD */
```

- `1` (default): Start the Eddystone-TLM scanner. No automatic advertising, so ST BLE Toolbox will not list the STM32 as a peripheral.
- `0`: Return to the generated BLE server advertising behavior, **without the scan thread**. This is a mode switch, not simultaneous advertising plus scanning.

Do not directly change `BLE_CFG_CENTRAL` in `ble_conf.h`; that change does not survive CubeMX regeneration when the Server Profile is selected.

## Validation performed

- Compared `app_ble.c` with the original `Phil_Vid_Test(2)` generated skeleton after removing the protected user-code content: **no changes outside user sections**.
- Simulated regeneration by replacing the original generated file's user-code content with this project's preserved sections: reproduced the new `app_ble.c` (normalizing line endings).
- Confirmed `ble_conf.h` matches the originally generated version and checked the `.ioc` heap, task stack and code-generator settings.
- **Not yet regenerated using the actual STM32CubeMX application**, built using the STM32 ARM toolchain, or tested on hardware. These checks reduce regeneration risk but are not a substitute for a build and hardware test.
