#include "main.h"

#define SENSOR_INTERVAL_MS 5000U
#define SHT41_ADDRESS     (0x44 << 1) // 0x88 (8-bit shifted address)
#define SHT41_MEASURE_CMD 0xE0        // High precision measurement command
#define STS41_I2C_ADDRESS         (0x45 << 1) // 0x88
#define STS41_MEASURE_HIGH_PREC   0xE0       // High precision measurement command

void SENSOR_Read_Data(I2C_HandleTypeDef hi2c1)
{
    uint8_t cmd = SHT41_MEASURE_CMD;
    uint8_t rx_buf[6] = {0};
    uint8_t rx_buf2[3] = {0};

    // 1. Send the measurement command to the sensor
    if (HAL_I2C_Master_Transmit(&hi2c1, SHT41_ADDRESS, &cmd, 1, HAL_MAX_DELAY) != HAL_OK)
    {
        // Handle transmission error
        return;
    }

    if (HAL_I2C_Master_Transmit(&hi2c1, STS41_I2C_ADDRESS, &cmd, 1,HAL_MAX_DELAY) != HAL_OK) {
		return;
	}

    // 2. Wait for the measurement conversion to finish (Max 8.2ms for high precision)
    HAL_Delay(4);

    // 3. Read 6 bytes of data back from the sensor
    if (HAL_I2C_Master_Receive(&hi2c1, SHT41_ADDRESS, rx_buf, 6, HAL_MAX_DELAY) != HAL_OK)
    {
        // Handle reception error
        return;
    }

    	// 3. Read 3 bytes back (MSB, LSB, Checksum)
	if (HAL_I2C_Master_Receive(&hi2c1, STS41_I2C_ADDRESS, rx_buf2, 3, HAL_MAX_DELAY) != HAL_OK) {
		return;
	}

    // 4. Convert raw data to physical values
//    raw_temperature = (rx_buf[0] << 8) | rx_buf[1];
    raw_temperature = (rx_buf2[0] << 8) | rx_buf2[1];
    raw_humidity  = (rx_buf[3] << 8) | rx_buf[4];

}

//void STS41_Read_Data(){
//	uint8_t cmd = STS41_MEASURE_HIGH_PREC;
//	uint8_t rx_buf[3] = { 0 };
//
//	// 1. Send the measurement command to the STS41
//	if (HAL_I2C_Master_Transmit(&hi2c1, STS41_I2C_ADDRESS, &cmd, 1,
//			HAL_MAX_DELAY) != HAL_OK) {
//		return;
//	}
//	// 2. Wait for the measurement to complete (High-precision takes ~10ms max)
//	HAL_Delay(14);
//
//	// 3. Read 3 bytes back (MSB, LSB, Checksum)
//	if (HAL_I2C_Master_Receive(&hi2c1, STS41_I2C_ADDRESS, rx_buf, 3,
//			HAL_MAX_DELAY) != HAL_OK) {
//		return;
//	}
//	// 4. Combine MSB and LSB into a raw 16-bit integer
//	raw_temperature = (rx_buf[0] << 8) | rx_buf[1];
////    HAL_Delay(1000);
//}

