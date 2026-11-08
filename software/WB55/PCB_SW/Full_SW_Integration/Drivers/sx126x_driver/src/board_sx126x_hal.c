/* SX1262 HAL SPI bridge: exclusively owned by the RadioControl FreeRTOS thread.
 * Adapted from FHSS_DEV; bounded BUSY and SPI timeouts, safe full-duplex reads.
 */
#include "sx126x_hal.h"
#include "main.h"
#include <string.h>

extern SPI_HandleTypeDef hspi1;
#define SX_NSS_LOW()  HAL_GPIO_WritePin(SX126X_NSS_GPIO_Port, SX126X_NSS_Pin, GPIO_PIN_RESET)
#define SX_NSS_HIGH() HAL_GPIO_WritePin(SX126X_NSS_GPIO_Port, SX126X_NSS_Pin, GPIO_PIN_SET)
#define SX_SPI_TIMEOUT_MS 100U
#define SX_BUSY_TIMEOUT_MS 100U

static bool wait_busy(void)
{
    uint32_t start = HAL_GetTick();
    while (HAL_GPIO_ReadPin(SX1262_BUSY_GPIO_Port, SX1262_BUSY_Pin) == GPIO_PIN_SET) {
        if ((HAL_GetTick() - start) >= SX_BUSY_TIMEOUT_MS) return false;
    }
    return true;
}

sx126x_hal_status_t sx126x_hal_write(const void *context, const uint8_t *command,
                                     uint16_t command_length, const uint8_t *data,
                                     uint16_t data_length)
{
    (void)context;
    if (!command || !command_length || (data_length && !data) || !wait_busy())
        return SX126X_HAL_STATUS_ERROR;
    SX_NSS_LOW();
    HAL_StatusTypeDef s = HAL_SPI_Transmit(&hspi1, (uint8_t *)command,
                                             command_length, SX_SPI_TIMEOUT_MS);
    if (s == HAL_OK && data_length)
        s = HAL_SPI_Transmit(&hspi1, (uint8_t *)data, data_length, SX_SPI_TIMEOUT_MS);
    SX_NSS_HIGH();
    if (s != HAL_OK) return SX126X_HAL_STATUS_ERROR;
    /* SetSleep (0x84) does not require a trailing BUSY wait. */
    return ((command[0] == 0x84U) || wait_busy()) ? SX126X_HAL_STATUS_OK
                                                   : SX126X_HAL_STATUS_ERROR;
}

sx126x_hal_status_t sx126x_hal_read(const void *context, const uint8_t *command,
                                    uint16_t command_length, uint8_t *data,
                                    uint16_t data_length)
{
    (void)context;
    if (!command || !command_length || (data_length && !data) || !wait_busy())
        return SX126X_HAL_STATUS_ERROR;
    SX_NSS_LOW();
    HAL_StatusTypeDef s = HAL_SPI_Transmit(&hspi1, (uint8_t *)command,
                                             command_length, SX_SPI_TIMEOUT_MS);
    /* Master 2-line HAL_SPI_Receive clocks out dummy bytes automatically. */
    if (s == HAL_OK && data_length) memset(data, 0, data_length);
    if (s == HAL_OK && data_length)
        s = HAL_SPI_Receive(&hspi1, data, data_length, SX_SPI_TIMEOUT_MS);
    SX_NSS_HIGH();
    return (s == HAL_OK && wait_busy()) ? SX126X_HAL_STATUS_OK : SX126X_HAL_STATUS_ERROR;
}

sx126x_hal_status_t sx126x_hal_reset(const void *context)
{
    (void)context;
    SX_NSS_HIGH();
    HAL_GPIO_WritePin(SX126X_RST_GPIO_Port, SX126X_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(SX126X_RST_GPIO_Port, SX126X_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(10);
    return wait_busy() ? SX126X_HAL_STATUS_OK : SX126X_HAL_STATUS_ERROR;
}

sx126x_hal_status_t sx126x_hal_wakeup(const void *context)
{
    (void)context;
    SX_NSS_LOW();
    HAL_Delay(1);
    SX_NSS_HIGH();
    HAL_Delay(1);
    return wait_busy() ? SX126X_HAL_STATUS_OK : SX126X_HAL_STATUS_ERROR;
}
