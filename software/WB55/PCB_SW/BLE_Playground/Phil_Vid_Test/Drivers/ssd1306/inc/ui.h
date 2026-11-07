#include <stdint.h>
#include <stdbool.h>
#include "ssd1306.h"

typedef enum
{
	SCREEN_MAIN_MENU = 0,
	SCREEN_RF,
	SCREEN_BLE,
	SCREEN_History
} ui_screen_t;

typedef enum
{
	MENU_RF = 0,
	MENU_BLE,
	MENU_HISTORY,
	MENU_COUNT
} main_menu_option_t;

typedef enum
{
	RF_CHANNELS = 0,
	RF_BW,
	RF_BACK,
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
void UI_RenderBLE(void);
void UI_RenderHistory(void);
void UI_HandleInput(bool up, bool down, bool left, bool right, bool select);
