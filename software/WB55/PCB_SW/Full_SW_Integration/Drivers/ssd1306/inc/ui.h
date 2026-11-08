#include <stdint.h>
#include <stdbool.h>
#include "ssd1306.h"
#include "ui_bridge.h"

typedef enum
{
	SCREEN_MAIN_MENU = 0,
	SCREEN_SETTINGS,
	SCREEN_BLE,
	SCREEN_History,
	SCREEN_HISTORY_DETAIL
} ui_screen_t;

typedef enum
{
	MENU_RF = 0,
	MENU_SETTINGS,
	MENU_HISTORY,
	MENU_COUNT
} main_menu_option_t;

typedef enum
{
	RF_CHANNELS = 0,
	RF_BW,
	RF_COUNT
} rf_menu_option_t;

typedef enum
{
	BLE_Test = 0,
	BLE_BACK,
	BLE_COUNT
} ble_menu_option_t;

typedef enum
{
	HISTORY_BACK = 0,
	HISTORY_COUNT
} history_menu_option_t;

void UI_Init(void);
void UI_Render(void);
void UI_RenderMainMenu(void);
void UI_RenderRF(void);
void UI_RenderSettings(void);
void UI_RenderHistory(void);
void UI_RenderHistoryDetail(void);
void UI_RenderBLE(void);
/* LEFT=previous, RIGHT=next, MID=select, DOWN=back. */
void UI_HandleInput(bool left, bool right, bool select, bool back);

void UI_SetBLEText(const char *text);

/* Call ONLY from UITask(): updates latest BLE value and 10-entry history. */
void UI_AddTLMReading(const ui_tlm_reading_t *reading);

/* Implemented by UI task, not by the BLE or SX1262 thread. */
bool UI_TakeRadioCommand(radio_command_t *out);
void UI_SetRadioStatus(const radio_report_t *report);
