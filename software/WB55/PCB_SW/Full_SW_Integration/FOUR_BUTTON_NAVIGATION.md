# V7 four-button UI navigation

Based on `Phil_Vid_Test_V7_DIO1_FreeRTOS.zip`.

## Physical input mapping

| GPIO | Button | Function |
|---|---|---|
| PA15 / EXTI15 | LEFT | Up / previous item or newer History record |
| PB5 / EXTI5 | RIGHT | Down / next item or older History record |
| PB4 / EXTI4 | MID | Select highlighted menu item, toggle Power, or cycle the highlighted FHSS setting |
| PB3 / EXTI3 | DOWN | Back to previous screen |
| PB6 | **Removed** | Not configured by CubeMX or serviced by the firmware |

All four button GPIOs use falling-edge interrupts with internal pull-ups; PA0 / EXTI0 is still SX1262 DIO1. `EXTI9_5_IRQn` stays enabled for PB5. The FreeRTOS button queue remains unchanged.

## Menu semantics

- **Main Menu:** LEFT/RIGHT wraps among Power, Settings, History. MID on Power submits the existing nonblocking SX1262 on/off command. DOWN has no effect because this is the root screen.
- **RF Settings:** LEFT/RIGHT chooses **Grid** or **Bandwidth**. MID toggles the LR-FHSS grid or cycles to the next valid bandwidth. DOWN returns to Main Menu. The obsolete on-screen **Back** row was removed because DOWN is dedicated to Back. As before, changes affect the next Power ON request, not a transmission currently in progress.
- **History:** LEFT/RIGHT moves the selection between the last ten stored records, newest first. MID opens a selected record's detail view. DOWN returns to Main Menu. In the detail view LEFT/RIGHT moves to adjacent records, and DOWN returns to History.
- **BLE text screen:** Remains an informational screen; DOWN returns to Main Menu. It is not reachable through the current three-option Main Menu, as in V7.

The history ring buffer, TLM advertisements, SX1262 FreeRTOS task, SPI, DIO1 ISR-to-task notification, and radio command queue are not changed.

## Updated project locations

- `Phil_Vid_Test.ioc`: PB6 / `BUTTON_UP` and EXTI6 pin mappings removed (all other pin mappings kept). Existing EXTI9_5 NVIC configuration retained for PB5.
- `Core/Inc/main.h`: `BUTTON_UP` macros removed.
- `Core/Src/main.c`: PB6 excluded from GPIO initialization, UP removed from the button enum and callbacks, UI task maps four input events to the new `UI_HandleInput(left,right,select,back)` API.
- `Core/Src/stm32wbxx_it.c`: EXTI9_5 handles PB5 only.
- `Drivers/ssd1306/inc/ui.h`: four-button API and History detail screen declarations.
- `Drivers/ssd1306/src/ui.c`: screen-agnostic Back, LEFT/RIGHT navigation, MID selection, RF setting cycling, History item selection and detail display.

## CubeMX regeneration

The input routing and FreeRTOS tasks are unchanged in structure. The `.ioc` and generated GPIO/EXTI files have matching changes. Custom UI files remain outside CubeMX-generated code. Modified `main.c` event handling is inside the existing USER CODE sections. Before regenerating, keep a project backup and **Keep User Code** enabled, then inspect the diff and rebuild.

## Verification

Host GCC syntax + behavior test passed for UI input processing: main menu wrapping, settings selection and value update, dedicated Back button, History selection and ten-record rollover, detail view, and Power command queue behavior. **Not** yet built with STM32CubeIDE ARM compiler, regenerated through CubeMX itself, or tested on STM32WB55 hardware.
