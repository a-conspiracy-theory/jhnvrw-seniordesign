/*
 * Sensors_File.h
 *
 *  Created on: Nov 4, 2026
 *      Author: Nicho
 */

#ifndef ENVIRONMENTAL_SENSORS_INC_SENSORS_FILE_H_
#define ENVIRONMENTAL_SENSORS_INC_SENSORS_FILE_H_

void SHT41_Read_Data(I2C_HandleTypeDef);
void STS41_Read_Data(I2C_HandleTypeDef);

#endif /* ENVIRONMENTAL_SENSORS_INC_SENSORS_FILE_H_ */
