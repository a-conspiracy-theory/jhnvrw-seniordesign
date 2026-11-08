BLE_HANDHELD_TRIAL1 Eddystone-TLM scanning port to Phil_Vid_Test(2)

Changed files:
  STM32_WPAN/App/ble_conf.h  Central role enabled alongside peripheral capability.
  STM32_WPAN/App/app_ble.c   Scan task, discovery restart notification,
                           advertising report parser, custom SHT41 decoding,
                           OLED update via existing UI_PostBLEText queue.

INSTALL: Copy both files over the matching paths in a BACKUP of Phil_Vid_Test(2),
then refresh/rebuild in STM32CubeIDE. Do not replace main.c, app_entry.c or ui.c.

Behavior: this first version is SCAN-ONLY; Phil no longer advertises as STM32WB
(and ST BLE Toolbox will not discover it as a connectable peripheral). The
existing server init and services remain in place for a later dual-role build.
It displays e.g. "H:7F00 T:6789" on the OLED BLE Settings page, which encodes
raw humidity/temperature as HEX from the custom beacon. The decoded values,
adv_count and time_since_boot are also in the debugger variable tlm_latest.

The scan function is run in a new small CMSIS-RTOS thread after APP_BLE_Init.
The existing HciUserEvtProcess continues to dispatch received reports.
No UTIL_SEQ calls or direct OLED writes were copied from the non-RTOS project.

Note: On some CPU2 firmware configurations, central+peripheral GAP roles may
require a compatible wireless-stack binary. A nonzero return from
aci_gap_init or aci_gap_start_general_discovery_proc should be investigated
before debugging the payload parser. Eddystone TLM broadcast is not a GATT
connection; the WB05KZ beacon does not need to be connected to.

Not hardware-tested or cross-compiled. CubeMX regeneration could overwrite
modifications in generated code outside USER CODE sections (advertising calls
and BLE role macro). Preserve/reapply these changes after regeneration.
