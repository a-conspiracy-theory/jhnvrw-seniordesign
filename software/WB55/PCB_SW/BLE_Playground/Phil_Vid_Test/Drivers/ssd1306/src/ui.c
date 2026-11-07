#include "ui.h"
#include <string.h>

/*
 * Display Dimensions
 */
#define SCREEN_WIDTH 	128
#define SCREEN_HEIGHT 	64
#define CHAR_HEIGHT 	10		// Only going to use Fonts_7x10 due to the screen size
#define CHAR_WIDTH		7

/*
 * Menu Configuration
 */
#define MENU_START_Y 		CHAR_HEIGHT + 2
#define MENU_SPACING 		12
#define MENU_ITEM_INDENT 	4

static main_menu_option_t 		selected_option = MENU_RF;
static rf_menu_option_t 		selected_rf_option = RF_CHANNELS;
static ble_menu_option_t 		selected_ble_option = BLE_BACK;
static history_menu_option_t 	selected_history_option = HISTORY_BACK;
static ui_screen_t 				current_screen = SCREEN_MAIN_MENU;

static const char *menu_items[MENU_COUNT] =
{
		"RF",
		"BLE",
		"History"
};

static const char *rf_items[RF_COUNT] =
{
		"Channels:",
		"Bandwidth:",
		"<-Back"
};

static const char *ble_items[BLE_COUNT] =
{
		"Test ID:",
		"<-Back"
};

static const char *history_items[HISTORY_COUNT] =
{
		"<-Back"
};

void UI_Init(void)
{
	selected_option = MENU_RF;
	UI_Render();
}

void UI_Render(void)
{
	SSD1306_Clear();

	switch(current_screen)
	{
	case SCREEN_MAIN_MENU:
		UI_RenderMainMenu();
		break;
	case SCREEN_RF:
		UI_RenderRF();
		break;
	case SCREEN_BLE:
		UI_RenderBLE();
		break;
	case SCREEN_History:
		UI_RenderHistory();
		break;
	default:
		current_screen = SCREEN_MAIN_MENU;
		UI_RenderMainMenu();
		break;
	}

	SSD1306_UpdateScreen();
}

void UI_RenderMainMenu(void)
{
	SSD1306_DrawRectangle(SCREEN_WIDTH/2, 1, SCREEN_WIDTH/2, SCREEN_HEIGHT-2, SSD1306_COLOR_WHITE);

	SSD1306_GotoXY(0, 0);
	SSD1306_Puts("Options:", &Font_7x10, SSD1306_COLOR_WHITE);


	for (uint8_t i=0; i<MENU_COUNT; i++)
	{
		uint8_t y = MENU_START_Y + (i * MENU_SPACING);

		if (i == selected_option)
		{
			SSD1306_DrawRectangle(0, y-1, SCREEN_WIDTH/2, 11, SSD1306_COLOR_WHITE);
		}

		SSD1306_GotoXY(MENU_ITEM_INDENT, y);
		SSD1306_Puts(menu_items[i], &Font_7x10, SSD1306_COLOR_WHITE);
	}

}

void UI_RenderRF(void)
{
	SSD1306_GotoXY(0, 0);
	SSD1306_Puts("RF Settings", &Font_7x10, SSD1306_COLOR_WHITE);

	for (uint8_t i=0; i<RF_COUNT; i++)
		{
			uint8_t y = MENU_START_Y + (i * MENU_SPACING);

			if (i == selected_rf_option)
			{
				SSD1306_DrawRectangle(0, y-1, SCREEN_WIDTH, 11, SSD1306_COLOR_WHITE);
			}

			SSD1306_GotoXY(MENU_ITEM_INDENT, y);
			SSD1306_Puts(rf_items[i], &Font_7x10, SSD1306_COLOR_WHITE);
		}

}

void UI_RenderBLE(void)
{
	SSD1306_GotoXY(0, 0);
	SSD1306_Puts("BLE Settings", &Font_7x10, SSD1306_COLOR_WHITE);

	for (uint8_t i=0; i<BLE_COUNT; i++)
		{
			uint8_t y = MENU_START_Y + (i * MENU_SPACING);

			if (i == selected_ble_option)
			{
				SSD1306_DrawRectangle(0, y-1, SCREEN_WIDTH, 11, SSD1306_COLOR_WHITE);
			}

			SSD1306_GotoXY(MENU_ITEM_INDENT, y);
			SSD1306_Puts(ble_items[i], &Font_7x10, SSD1306_COLOR_WHITE);
		}
}

void UI_RenderHistory(void)
{
	SSD1306_GotoXY(0, 0);
	SSD1306_Puts("Sensor History", &Font_7x10, SSD1306_COLOR_WHITE);

	for (uint8_t i=0; i<HISTORY_COUNT; i++)
		{
			uint8_t y = MENU_START_Y + (i * MENU_SPACING);

			if (i == selected_history_option)
			{
				SSD1306_DrawRectangle(0, y-1, SCREEN_WIDTH, 11, SSD1306_COLOR_WHITE);
			}

			SSD1306_GotoXY(MENU_ITEM_INDENT, y);
			SSD1306_Puts(history_items[i], &Font_7x10, SSD1306_COLOR_WHITE);
		}
}

void UI_HandleInput(bool up, bool down, bool left, bool right, bool select)
{
	bool changed = false;

	switch (current_screen)
	{
		case SCREEN_MAIN_MENU:
			if (up)
			{
				if (selected_option == 0)
					selected_option = MENU_COUNT - 1;
				else
					selected_option--;

				changed = true;
			}

			if (down)
			{
				selected_option++;

				if (selected_option >= MENU_COUNT)
					selected_option = 0;

				changed = true;
			}

			if (select)
			{
				switch (selected_option)
				{
				case MENU_RF:
					current_screen = SCREEN_RF;
					break;
				case MENU_BLE:
					current_screen = SCREEN_BLE;
					break;
				case MENU_HISTORY:
					current_screen = SCREEN_History;
					break;
				default:
					break;
				}

				changed = true;
			}
			break;

		case SCREEN_RF:
			if (up)
			{
				if (selected_rf_option == 0)
					selected_rf_option = RF_COUNT - 1;
				else
					selected_rf_option--;

				changed = true;
			}

			if (down)
			{
				selected_rf_option++;

				if (selected_rf_option >= RF_COUNT)
					selected_rf_option = 0;

				changed = true;
			}

			if (select)
			{
				switch (selected_rf_option)
				{
				case RF_CHANNELS:
					break;
				case RF_BW:
					break;
				case RF_BACK:
					current_screen = SCREEN_MAIN_MENU;
					changed = true;
					break;
				default:
					break;
				}
			}
			break;

		case SCREEN_BLE:
			if (up)
			{
				if (selected_ble_option == 0)
					selected_ble_option = BLE_COUNT - 1;
				else
					selected_ble_option--;

				changed = true;
			}

			if (down)
			{
				selected_ble_option++;

				if (selected_ble_option >= BLE_COUNT)
					selected_ble_option = 0;

				changed = true;
			}

			if (select)
			{
				switch (selected_ble_option)
				{
				case BLE_BACK:
					current_screen = SCREEN_MAIN_MENU;
					changed = true;
					break;
				default:
					break;
				}
			}
			break;

		case SCREEN_History:
			if (up)
			{
				if (selected_history_option == 0)
					selected_history_option = HISTORY_COUNT - 1;
				else
					selected_history_option--;

				changed = true;
			}

			if (down)
			{
				selected_history_option++;

				if (selected_history_option >= HISTORY_COUNT)
					selected_history_option = 0;

				changed = true;
			}

			if (select)
			{
				switch (selected_history_option)
				{
				case HISTORY_BACK:
					current_screen = SCREEN_MAIN_MENU;
					changed = true;
					break;
				default:
					break;
				}
			}
	}



	if (changed)
	{
		UI_Render();
	}
}
