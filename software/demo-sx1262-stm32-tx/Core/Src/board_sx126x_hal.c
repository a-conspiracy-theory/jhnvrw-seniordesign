#include "sx126x_hal.h"
#include "string.h"
#include "main.h"

extern SPI_HandleTypeDef hspi1;

#define SX126X_CS_LOW() \
	HAL_GPIO_WritePin(SX1262_NSS_GPIO_Port, SX1262_NSS_Pin, GPIO_PIN_RESET)

#define SX126X_CS_HIGH() \
	HAL_GPIO_WritePin(SX1262_NSS_GPIO_Port, SX1262_NSS_Pin, GPIO_PIN_SET)

bool sx126x_wait_on_busy(void)
{
	uint32_t start = HAL_GetTick();

	while(HAL_GPIO_ReadPin(
			SX1262_BUSY_GPIO_Port,
			SX1262_BUSY_Pin
		) == GPIO_PIN_SET)
	{
		// wait
		if((HAL_GetTick() - start) > 100)
		{
			return false;
		}
	}
	return true;
}

sx126x_hal_status_t sx126x_hal_write(const void* context, const uint8_t* command, const uint16_t command_length,
		const uint8_t* data, const uint16_t data_length)
{

	sx126x_wait_on_busy();
	// NSS goes low to select the chip over SPI
	SX126X_CS_LOW();

	// OP Code Byte
	if(HAL_SPI_Transmit(&hspi1, (uint8_t*)command, command_length, HAL_MAX_DELAY) != HAL_OK)
	{
		SX126X_CS_HIGH();
		return SX126X_HAL_STATUS_ERROR;
	}

	// Parameter Byte
	if(data_length > 0)
	{
		if(HAL_SPI_Transmit(&hspi1, (uint8_t*) data, data_length, HAL_MAX_DELAY) != HAL_OK)
		{
			SX126X_CS_HIGH();
			return SX126X_HAL_STATUS_ERROR;
		}
	}

	SX126X_CS_HIGH();

	sx126x_wait_on_busy();

	return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_read(const void* context, const uint8_t* command, uint16_t command_length,
		uint8_t* data, uint16_t data_length)
{
	// max transaction = 255-byte payload + opcode + address + dummy = 260 bytes

	uint8_t tx[256] = {0};
	uint8_t rx[256] = {0};

	uint16_t total = command_length + data_length;

	sx126x_wait_on_busy();

	memcpy(tx, command, command_length);
	//memset(&tx[command_length], 0x00, data_length);

	SX126X_CS_LOW();

	//tx_buffer[0] = command[0];


	if(HAL_SPI_TransmitReceive(&hspi1, tx, rx, total, HAL_MAX_DELAY) != HAL_OK)
	{
		SX126X_CS_HIGH();
		return SX126X_HAL_STATUS_ERROR;
	}

	SX126X_CS_HIGH();

	memcpy(data, &rx[command_length], data_length);

	sx126x_wait_on_busy();

	return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_reset(const void* context)
{

	HAL_GPIO_WritePin(
			SX1262_RST_GPIO_Port,
			SX1262_RST_Pin,
			GPIO_PIN_RESET
	);

	HAL_Delay(1);

	HAL_GPIO_WritePin(
			SX1262_RST_GPIO_Port,
			SX1262_RST_Pin,
			GPIO_PIN_SET
	);

	HAL_Delay(10);

	sx126x_wait_on_busy();

	return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_wakeup(const void* context)
{
	SX126X_CS_LOW();

	HAL_Delay(1);

	SX126X_CS_HIGH();

	HAL_Delay(1);

	sx126x_wait_on_busy();

	return SX126X_HAL_STATUS_OK;
}


