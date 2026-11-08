# Phil_Vid_Test_V6: SX1262 LR-FHSS on FreeRTOS

This project is based on the user's **Phil_Vid_Test_V6** source and ports
Semtech's `sx126x_driver` and FHSS configuration from **FHSS_DEV**.

## New files

- `Drivers/sx126x_driver/inc/` and `src/`: Semtech SX126x / LR-FHSS library files
  copied from FHSS_DEV; reworked `board_sx126x_calls.*` and `board_sx126x_hal.c`.
- `Core/Inc/radio_control.h`, `Core/Src/radio_control.c`: FreeRTOS radio worker.
- `Core/Inc/ui_bridge.h`: UI queue event type extended for radio status reports.

## Changed existing files

- `Core/Src/main.c`: creates radio task from `USER CODE BEGIN RTOS_THREADS`;
  routes Power toggles via `RadioControl_Request` and status updates via `UI_Queue`;
  configures SPI1 for **8 bits**, prescaler 16 (~2 MHz at 32 MHz SPI clock),
  and starts NSS PB2 HIGH.
- `Drivers/ssd1306/src/ui.c`, `inc/ui.h`: Settings now edits LR-FHSS grid
  and bandwidth, Power toggles radio TX mode on/off, History / BLE untouched.
- `Phil_Vid_Test.ioc`: SPI1 8-bit, prescaler 16, NSS PB2 starts high,
  SPI1 init in function order, keeps existing SPI and GPIO pins.
- `.cproject`: adds `Drivers/sx126x_driver/inc` to include paths for Debug/Release.

## Wiring and hardware assumptions

The `.ioc` provided in V6 defines:

| Signal | STM32WB55CCU6 pin |
| --- | --- |
| SPI1 SCK | PA1 |
| SPI1 MISO | PA6 |
| SPI1 MOSI | PA7 |
| SX1262 BUSY | PA2 |
| SX1262 DIO2_CPS | PA3 |
| SX1262 RESET | PA4 |
| RF switch TXEN | PA5 |
| SPI NSS (software CS) | PB2 |
| RF switch RXEN | PB7 |

**IMPORTANT: The V6 `.ioc` assigns no SX1262 DIO1 pin.** The radio task
therefore polls SX1262 IRQ status every RTOS tick (nominally 1 ms) while a
transmission is underway. This is a functional first-pass fallback, but RF
hopping can miss deadlines if BLE / other tasks preempt the polling thread.
For reliable FHSS hardware, wire DIO1 to a free EXTI-capable pin, configure
it in CubeMX, and replace polling with an ISR -> task notification scheme.
The FHSS_DEV DIO1 interrupt setup cannot be transplanted directly to V6.

**Confirm RF switch polarities on your PCB.** Code follows FHSS_DEV:
TXEN=HIGH and RXEN=LOW during TX, both LOW when off. PA3 DIO2_CPS is
left in the initial CubeMX state as in FHSS_DEV.

## FreeRTOS ownership

- `RadioControl_Create()` is called after CMSIS `osKernelInitialize()` and
  before `osKernelStart()`.
- `SX1262_FHSS` FreeRTOS thread (AboveNormal, 2048-byte stack) is the ONLY
  caller of SX1262 SPI and Semtech driver commands. No interrupt or UI thread
  performs SPI transactions.
- `radio_queue` carries start/stop commands with a **snapshot** of the menu's
  LR-FHSS grid and bandwidth. `UI_Queue` carries back a success/failure state.
- Existing BLE scanning thread / BLE HCI processing and OLED history remain
  unchanged. The radio task coexists with them, subject to MCU timing limits.

## Menu / button behavior

- Main menu **Power**: press MID to start transmitting using the currently
  selected parameters. Press MID again to stop TX and enter SX1262 cold sleep.
  The radio is *not* physically power-gated by a GPIO switch. The right side
  of the main menu shows `TX OFF`, `TX...`, `TX ON`, or `TX ERR`.
- **Settings**: UP/DOWN highlight Grid, BW, Back. RIGHT/LEFT cycle the
  available values; MID also cycles the highlighted Grid/BW entry; select
  Back with MID to exit. The menu uses Semtech's enums directly.
- 25.391 kHz grid supports BW choices 722.656k, 773.438k, 1.523M, 1.574M;
  the underlying Semtech driver rejects BW values below 722.656k with this
  grid. The 3.906 kHz grid exposes all 10 listed bandwidth choices.
- Changes made while TX is ON are staged for the **next** power-on. Turn
  Power OFF then ON to apply them. Defaults match FHSS_DEV:
  Grid 25.391 kHz; bandwidth 1.523438 MHz.
- OLED History and 10-entry Eddystone-TLM BLE log are retained.

## FHSS transmitter behavior

- Center frequency: **915 MHz**.
- Modulation: GMSK 488, CR 1/3, 3 headers, sync word
  `{0x2C,0x0F,0x79,0x95}`, pseudo-random hop sequence deck.
- TX output setting: **+22 dBm** (the value in FHSS_DEV; check radio module
  limits, antenna, local RF regulations, EIRP, and test setup before enabling).
- Repeated test payload: ASCII `FHSS_V6` because FHSS_DEV's example `main.c`
  references `payload` without declaring it. You can change
  `demo_payload[]` in `Core/Src/radio_control.c`.
- Gap between successful TX_DONE and next packet: 50 ms.
- Power OFF: aborts TX with STANDBY, drives RF switch TXEN/RXEN low, then
  sends the SX1262 COLD_START SLEEP command. Power ON resets and reconfigures
  the chip. Changing grid/BW while TX is on does NOT touch the radio until
  the next power-on.
- If SPI / radio operations fail, the worker reports `RADIO_STATE_ERROR` to
  the UI and stops sending. The `radio_report()` function is a good breakpoint
  for inspecting the error code.

## Validation performed (host-side)

- GCC syntax-only checks for all newly added/ported driver C files, radio
  control module, UI module, and modified `main.c` using local stub for
  the cross-toolchain's newlib `reent.h`.
- UI command simulation confirmed Power request, pending-command suppression,
  Settings grid/BW selection, automatic invalid-grid/BW correction, and
  subsequent Power OFF/ON requests.
- `.cproject` XML parsed successfully; `.ioc` updated to match generated SPI
  configuration and default NSS level.

**Not performed:** full ARM cross-compile/link in STM32CubeIDE, hardware
flash/test, RF timing validation, RF output-power measurement, verification
of the selected bandwidths on an actual SX1262, or an actual CubeMX
regeneration pass. Review build logs and on-board behavior before treating
this as production-ready or guaranteed hopping timing.

## CubeMX regeneration

The custom task creation code, UI event routing, and includes added to
`Core/Src/main.c` are inside existing `USER CODE` sections. The generated SPI
and GPIO statements were updated alongside the corresponding `.ioc` entries.
The Semtech library and radio control files are separate, non-generated
sources. Back up the working project and **check both Debug/Release include
paths** in the C/C++ Build configuration after CubeMX regenerates the project:
`Drivers/sx126x_driver/inc`. The `.ioc` file does not natively represent
that custom include directory, so the `.cproject` addition may need
reapplying after regeneration. The `.ioc` retains `KeepUserCode=true` and
`DeletePrevious=false`.

## First hardware checks

1. Import and clean/rebuild the project in CubeIDE.
2. Check radio starts in OFF state; main menu displays `TX OFF`.
3. Inspect GPIOs NSS=HIGH, TXEN=LOW, RXEN=LOW before enabling.
4. Enter Settings and verify grid/BW cycling; select defaults first.
5. Test TX using properly terminated, appropriately attenuated measurement
   equipment; observe SPI and BUSY, then verify hop/TxDone interrupt status.
6. If TX never finishes or reports an error, inspect return values at
   `fhss_radio_configure()`, `fhss_radio_send()`, `fhss_radio_poll()`.
7. Before relying on timing-sensitive hopping, connect DIO1 and replace
   polling with an IRQ-driven wakeup.
