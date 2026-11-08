#include "ui.h"
#include <string.h>
#include <stdio.h>

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
static ui_screen_t 				current_screen = SCREEN_MAIN_MENU;
static char 					ble_text[UI_BLE_TEXT_MAX + 1] = "";

static const char *menu_items[MENU_COUNT] =
{
		"Power",
		"Settings",
		"History"
};

/* The settings labels are generated from the actual LR-FHSS enum values. */
static lr_fhss_v1_grid_t selected_grid = LR_FHSS_V1_GRID_25391_HZ;
static lr_fhss_v1_bw_t selected_bw = LR_FHSS_V1_BW_1523438_HZ;
static bool radio_on = false;
static bool radio_pending = false;
static bool radio_error = false;
static bool radio_request_ready = false;
static radio_command_t pending_radio_command;

static const char *const bandwidth_names[10] = {
    "39.063k", "85.938k", "136.719k", "183.594k", "335.938k",
    "386.719k", "722.656k", "773.438k", "1.523M", "1.574M"
};

static uint8_t first_bandwidth(void)
{
    /* Semtech LR-FHSS driver requires BW >= 722.656k for 25.391k grid. */
    return selected_grid == LR_FHSS_V1_GRID_25391_HZ ? 6U : 0U;
}

static void step_bandwidth(bool forward)
{
    uint8_t bw = (uint8_t)selected_bw;
    const uint8_t first = first_bandwidth();
    if (forward) bw = (bw >= 9U) ? first : (uint8_t)(bw + 1U);
    else bw = (bw <= first) ? 9U : (uint8_t)(bw - 1U);
    selected_bw = (lr_fhss_v1_bw_t)bw;
}

static void step_grid(void)
{
    selected_grid = (selected_grid == LR_FHSS_V1_GRID_25391_HZ)
                    ? LR_FHSS_V1_GRID_3906_HZ : LR_FHSS_V1_GRID_25391_HZ;
    if ((uint8_t)selected_bw < first_bandwidth())
        selected_bw = LR_FHSS_V1_BW_722656_HZ;
}


/* Newest-first circular buffer; only UITask() accesses this UI state. */
#define UI_HISTORY_CAPACITY 10U
#define UI_HISTORY_VISIBLE  2U

static ui_tlm_reading_t history[UI_HISTORY_CAPACITY];
static uint8_t history_next = 0U;  /* index of next write */
static uint8_t history_count = 0U;
static uint8_t history_first = 0U; /* newest-relative index at top of page */
static uint8_t history_selected = 0U; /* newest-relative highlighted record */

/* SHT4x sensor conversion: Sensirion datasheet, section 4.6.
 * Keep raw readings in the history; convert them only when rendering.
 * The receiver's 'humidity_raw' is carried in the beacon's TLM battery field.
 */
static float UI_TemperatureC(uint16_t raw)
{
    return -45.0f + (315.0f * ((float)raw / 65535.0f));
}

static float UI_HumidityPercent(uint16_t raw)
{
    float rh = -6.0f + (125.0f * ((float)raw / 65535.0f));
    if (rh < 0.0f) rh = 0.0f;
    if (rh > 100.0f) rh = 100.0f;
    return rh;
}

/* Rounding to one decimal without using printf's optional float support. */
static int32_t UI_ToTenths(float value)
{
    return (int32_t)(value * 10.0f + (value >= 0.0f ? 0.5f : -0.5f));
}

static void UI_FormatTemperature(char *out, size_t cap, uint16_t raw)
{
    int32_t t = UI_ToTenths(UI_TemperatureC(raw));
    uint32_t magnitude = (uint32_t)(t < 0 ? -t : t);
    (void)snprintf(out, cap, "%s%lu.%luF", t < 0 ? "-" : "",
                   (unsigned long)(magnitude / 10U),
                   (unsigned long)(magnitude % 10U));
}

static void UI_FormatHumidity(char *out, size_t cap, uint16_t raw)
{
    uint32_t rh = (uint32_t)UI_ToTenths(UI_HumidityPercent(raw));
    (void)snprintf(out, cap, "%lu.%lu%%",
                   (unsigned long)(rh / 10U),
                   (unsigned long)(rh % 10U));
}

static const ui_tlm_reading_t *UI_HistoryAt(uint8_t newest_relative_index)
{
    uint8_t index = (uint8_t)((history_next + UI_HISTORY_CAPACITY - 1U -
                              newest_relative_index) % UI_HISTORY_CAPACITY);
    return &history[index];
}


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
	case SCREEN_SETTINGS:
		UI_RenderSettings();
		break;
	case SCREEN_BLE:
		UI_RenderBLE();
		break;
	case SCREEN_History:
		UI_RenderHistory();
		break;
	case SCREEN_HISTORY_DETAIL:
		UI_RenderHistoryDetail();
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
    SSD1306_DrawRectangle(SCREEN_WIDTH/2, 1, SCREEN_WIDTH/2,
                          SCREEN_HEIGHT-2, SSD1306_COLOR_WHITE);

    SSD1306_GotoXY(0, 0);
    SSD1306_Puts("Options:", &Font_7x10, SSD1306_COLOR_WHITE);

    for (uint8_t i = 0; i < MENU_COUNT; i++)
    {
        uint8_t y = MENU_START_Y + (i * MENU_SPACING);
        if (i == selected_option)
        {
            SSD1306_DrawRectangle(0, y-1, SCREEN_WIDTH/2, 11,
                                  SSD1306_COLOR_WHITE);
        }
        SSD1306_GotoXY(MENU_ITEM_INDENT, y);
        SSD1306_Puts((char *)menu_items[i], &Font_7x10, SSD1306_COLOR_WHITE);
    }

    SSD1306_GotoXY(68, 1);
    SSD1306_Puts(radio_pending ? "TX..." :
                 (radio_error ? "TX ERR" : (radio_on ? "TX ON" : "TX OFF")),
                 &Font_7x10, SSD1306_COLOR_WHITE);

    /* Right side: latest accepted Eddystone-TLM sensor sample.
     * This screen is rendered only by UITask(), so no extra mutex is needed.
     */

    if (history_count == 0U)
    {
        SSD1306_GotoXY(68, 16);
        SSD1306_Puts("--.-F", &Font_11x18, SSD1306_COLOR_WHITE);
        SSD1306_GotoXY(68, SCREEN_HEIGHT/2 + 4);
        SSD1306_Puts("--.-%", &Font_11x18, SSD1306_COLOR_WHITE);
    }
    else
    {
        char value[16];
        const ui_tlm_reading_t *latest = UI_HistoryAt(0U);
        UI_FormatTemperature(value, sizeof(value), latest->temperature_raw);
        SSD1306_GotoXY(68, 16);
        SSD1306_Puts(value, &Font_11x18, SSD1306_COLOR_WHITE);
        UI_FormatHumidity(value, sizeof(value), latest->humidity_raw);
        SSD1306_GotoXY(68, 44);
        SSD1306_Puts(value, &Font_11x18, SSD1306_COLOR_WHITE);
    }
}

void UI_RenderRF(void)
{

}

void UI_RenderSettings(void)
{
    char grid_line[22], bw_line[22];
    (void)snprintf(grid_line, sizeof(grid_line), "Grid:%s",
                   selected_grid == LR_FHSS_V1_GRID_25391_HZ ? "25.391k" : "3.906k");
    (void)snprintf(bw_line, sizeof(bw_line), "BW:%s", bandwidth_names[(uint8_t)selected_bw]);
    const char *rows[RF_COUNT] = {grid_line, bw_line};

    SSD1306_GotoXY(0, 0);
    SSD1306_Puts("RF Settings", &Font_7x10, SSD1306_COLOR_WHITE);
    for (uint8_t i = 0; i < RF_COUNT; ++i)
    {
        uint8_t y = (uint8_t)(MENU_START_Y + i * MENU_SPACING);
        if (selected_rf_option == i)
            SSD1306_DrawRectangle(0, y - 1, SCREEN_WIDTH, 11, SSD1306_COLOR_WHITE);
        SSD1306_GotoXY(MENU_ITEM_INDENT, y);
        SSD1306_Puts((char *)rows[i], &Font_7x10, SSD1306_COLOR_WHITE);
    }
    SSD1306_GotoXY(0, 40);
    SSD1306_Puts("MID:Change", &Font_7x10, SSD1306_COLOR_WHITE);
    SSD1306_GotoXY(0, 52);
    SSD1306_Puts("DOWN:Back", &Font_7x10, SSD1306_COLOR_WHITE);
}

/* The BLE text screen is currently informational; DOWN always goes back. */
void UI_RenderBLE(void)
{
    SSD1306_GotoXY(0, 0);
    SSD1306_Puts("BLE Settings", &Font_7x10, SSD1306_COLOR_WHITE);
    SSD1306_GotoXY(0, 15);
    SSD1306_Puts("Test ID:", &Font_7x10, SSD1306_COLOR_WHITE);
    SSD1306_GotoXY(0, 28);
    SSD1306_Puts(ble_text, &Font_7x10, SSD1306_COLOR_WHITE);
    SSD1306_GotoXY(0, 52);
    SSD1306_Puts("DOWN:Back", &Font_7x10, SSD1306_COLOR_WHITE);
}

void UI_RenderHistory(void)
{
    char line[24];

    if (history_count == 0U)
    {
        SSD1306_GotoXY(0, 0);
        SSD1306_Puts("Sensor History", &Font_7x10, SSD1306_COLOR_WHITE);
        SSD1306_GotoXY(4, 18);
        SSD1306_Puts("No readings yet", &Font_7x10, SSD1306_COLOR_WHITE);
        SSD1306_GotoXY(4, 42);
        SSD1306_Puts("DOWN:Back", &Font_7x10, SSD1306_COLOR_WHITE);
        return;
    }

    uint8_t visible = (uint8_t)(history_count - history_first);
    if (visible > UI_HISTORY_VISIBLE)
        visible = UI_HISTORY_VISIBLE;

    /* Example: History 01-02/10. LEFT/RIGHT scroll the records. */
    (void)snprintf(line, sizeof(line), "History %02u-%02u/%02u",
                   (unsigned)(history_first + 1U),
                   (unsigned)(history_first + visible),
                   (unsigned)history_count);
    SSD1306_GotoXY(0, 0);
    SSD1306_Puts(line, &Font_7x10, SSD1306_COLOR_WHITE);

    for (uint8_t i = 0U; i < visible; i++)
    {
        uint8_t item_number = (uint8_t)(history_first + i + 1U);
        const ui_tlm_reading_t *reading = UI_HistoryAt(history_first + i);
        uint32_t total = reading->received_seconds;
        uint32_t hours = total / 3600U;
        uint32_t minutes = (total / 60U) % 60U;
        uint32_t seconds = total % 60U;
        uint8_t y = (uint8_t)(12U + i * 23U);
        if (history_selected == (uint8_t)(history_first + i))
            SSD1306_DrawRectangle(0, (uint8_t)(y - 1U), SCREEN_WIDTH, 22U,
                                  SSD1306_COLOR_WHITE);

        /* Timestamp = elapsed STM32 uptime, not wall-clock or beacon uptime. */
        (void)snprintf(line, sizeof(line), "%02u %02lu:%02lu:%02lu",
                       (unsigned)item_number,
                       (unsigned long)hours,
                       (unsigned long)minutes,
                       (unsigned long)seconds);
        SSD1306_GotoXY(2, y);
        SSD1306_Puts(line, &Font_7x10, SSD1306_COLOR_WHITE);

        /* Render engineering units instead of raw sensor words. */
        char temp[16], rh[16];
        UI_FormatTemperature(temp, sizeof(temp), reading->temperature_raw);
        UI_FormatHumidity(rh, sizeof(rh), reading->humidity_raw);
        (void)snprintf(line, sizeof(line), "T:%.7s H:%.6s", temp, rh);
        SSD1306_GotoXY(2, (uint8_t)(y + 11U));
        SSD1306_Puts(line, &Font_7x10, SSD1306_COLOR_WHITE);
    }
}

/* Navigate the 10-entry history, keeping the selected entry visible. */
static void UI_StepHistory(bool toward_older)
{
    if (history_count == 0U)
        return;

    if (toward_older)
    {
        if ((uint8_t)(history_selected + 1U) < history_count)
            history_selected++;
    }
    else if (history_selected > 0U)
    {
        history_selected--;
    }

    if (history_selected < history_first)
        history_first = history_selected;
    else if (history_selected >= (uint8_t)(history_first + UI_HISTORY_VISIBLE))
        history_first = (uint8_t)(history_selected - UI_HISTORY_VISIBLE + 1U);
}

/* LEFT: scroll up; RIGHT: scroll down; MID: select; DOWN: back. */
void UI_HandleInput(bool left, bool right, bool select, bool back)
{
    if (back)
    {
        /* The main menu is the root: DOWN has no action there. */
        if (current_screen == SCREEN_HISTORY_DETAIL)
            current_screen = SCREEN_History;
        else if (current_screen != SCREEN_MAIN_MENU)
            current_screen = SCREEN_MAIN_MENU;
        return;
    }

    switch (current_screen)
    {
    case SCREEN_MAIN_MENU:
        if (left)
            selected_option = (selected_option == 0)
                              ? (MENU_COUNT - 1) : (selected_option - 1);
        else if (right)
            selected_option = (selected_option + 1) % MENU_COUNT;

        if (select)
        {
            switch (selected_option)
            {
            case MENU_RF:
                /* Queue the desired state; UITask never touches SPI. */
                if (!radio_pending)
                {
                    pending_radio_command.enable = !radio_on;
                    pending_radio_command.grid = selected_grid;
                    pending_radio_command.bw = selected_bw;
                    radio_request_ready = true;
                    radio_pending = true;
                    radio_error = false;
                }
                break;
            case MENU_SETTINGS:
                selected_rf_option = RF_CHANNELS;
                current_screen = SCREEN_SETTINGS;
                break;
            case MENU_HISTORY:
                history_first = 0U;
                history_selected = 0U;
                current_screen = SCREEN_History;
                break;
            default:
                break;
            }
        }
        break;

    case SCREEN_SETTINGS:
        if (left)
            selected_rf_option = (selected_rf_option == 0)
                                 ? (RF_COUNT - 1) : (selected_rf_option - 1);
        else if (right)
            selected_rf_option = (selected_rf_option + 1) % RF_COUNT;

        if (select)
        {
            if (selected_rf_option == RF_CHANNELS)
                step_grid();
            else if (selected_rf_option == RF_BW)
                step_bandwidth(true);
        }
        break;

    case SCREEN_History:
        if (left)
            UI_StepHistory(false);
        else if (right)
            UI_StepHistory(true);
        if (select && history_count > 0U)
            current_screen = SCREEN_HISTORY_DETAIL;
        break;

    case SCREEN_HISTORY_DETAIL:
        if (left)
            UI_StepHistory(false);
        else if (right)
            UI_StepHistory(true);
        break;

    case SCREEN_BLE:
        /* Informational screen: no selectable items at present. */
        break;
    default:
        break;
    }
}

void UI_RenderHistoryDetail(void)
{
    char line[25];
    if (history_count == 0U)
    {
        SSD1306_GotoXY(0, 0);
        SSD1306_Puts("No readings", &Font_7x10, SSD1306_COLOR_WHITE);
        return;
    }

    const ui_tlm_reading_t *reading = UI_HistoryAt(history_selected);
    uint32_t total = reading->received_seconds;
    char temp[16], rh[16];
    UI_FormatTemperature(temp, sizeof(temp), reading->temperature_raw);
    UI_FormatHumidity(rh, sizeof(rh), reading->humidity_raw);

    (void)snprintf(line, sizeof(line), "Reading %02u/%02u",
                   (unsigned)(history_selected + 1U), (unsigned)history_count);
    SSD1306_GotoXY(0, 0);
    SSD1306_Puts(line, &Font_7x10, SSD1306_COLOR_WHITE);

    (void)snprintf(line, sizeof(line), "Time %02lu:%02lu:%02lu",
                   (unsigned long)(total / 3600U),
                   (unsigned long)((total / 60U) % 60U),
                   (unsigned long)(total % 60U));
    SSD1306_GotoXY(0, 12);
    SSD1306_Puts(line, &Font_7x10, SSD1306_COLOR_WHITE);

    (void)snprintf(line, sizeof(line), "Temp: %s", temp);
    SSD1306_GotoXY(0, 24);
    SSD1306_Puts(line, &Font_7x10, SSD1306_COLOR_WHITE);
    (void)snprintf(line, sizeof(line), "RH: %s", rh);
    SSD1306_GotoXY(0, 36);
    SSD1306_Puts(line, &Font_7x10, SSD1306_COLOR_WHITE);
    SSD1306_GotoXY(0, 52);
    SSD1306_Puts("DOWN:Back", &Font_7x10, SSD1306_COLOR_WHITE);
}

void UI_SetBLEText(const char *text)
{
    if (text == NULL)
        return;

    strncpy(ble_text, text, UI_BLE_TEXT_MAX);
    ble_text[UI_BLE_TEXT_MAX] = '\0';
}

void UI_AddTLMReading(const ui_tlm_reading_t *reading)
{
    if (reading == NULL)
        return;

    history[history_next] = *reading;
    history_next = (uint8_t)((history_next + 1U) % UI_HISTORY_CAPACITY);
    if (history_count < UI_HISTORY_CAPACITY)
        history_count++;

    /* Keep approximately the same older records on screen when new data
     * arrives while the user is browsing.  The oldest is overwritten at 10. */
    if ((current_screen == SCREEN_History || current_screen == SCREEN_HISTORY_DETAIL) &&
        history_selected > 0U)
    {
        /* Keep the same older reading selected while new samples arrive. */
        if (history_selected < (uint8_t)(history_count - 1U))
            history_selected++;
        if (history_selected >= (uint8_t)(history_first + UI_HISTORY_VISIBLE))
            history_first = (uint8_t)(history_selected - UI_HISTORY_VISIBLE + 1U);
        else if (history_selected < history_first)
            history_first = history_selected;
    }

    /* Latest reading is now rendered from the ring buffer on each screen.
     * The BLE text buffer remains available for the separate write command. */
}

/* Only UITask() calls these UI APIs. Requests are copied into RadioControl's queue. */
bool UI_TakeRadioCommand(radio_command_t *out)
{
    if (!out || !radio_request_ready) return false;
    *out = pending_radio_command;
    radio_request_ready = false;
    return true;
}

void UI_SetRadioStatus(const radio_report_t *report)
{
    if (!report) return;
    radio_pending = false;
    radio_on = report->state == RADIO_STATE_ON;
    radio_error = report->state == RADIO_STATE_ERROR;
}
