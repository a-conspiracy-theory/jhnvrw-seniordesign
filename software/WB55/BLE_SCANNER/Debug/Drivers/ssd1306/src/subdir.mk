################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/ssd1306/src/fonts.c \
../Drivers/ssd1306/src/ssd1306.c 

OBJS += \
./Drivers/ssd1306/src/fonts.o \
./Drivers/ssd1306/src/ssd1306.o 

C_DEPS += \
./Drivers/ssd1306/src/fonts.d \
./Drivers/ssd1306/src/ssd1306.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/ssd1306/src/%.o Drivers/ssd1306/src/%.su Drivers/ssd1306/src/%.cyclo: ../Drivers/ssd1306/src/%.c Drivers/ssd1306/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32WB55xx -c -I../Core/Inc -I../Drivers/STM32WBxx_HAL_Driver/Inc -I../Drivers/STM32WBxx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32WBxx/Include -I../Drivers/CMSIS/Include -I../STM32_WPAN/App -I../Utilities/lpm/tiny_lpm -I../Middlewares/ST/STM32_WPAN -I../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread -I../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl -I../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/shci -I../Middlewares/ST/STM32_WPAN/utilities -I../Middlewares/ST/STM32_WPAN/ble/core -I../Middlewares/ST/STM32_WPAN/ble/core/auto -I../Middlewares/ST/STM32_WPAN/ble/core/template -I../Middlewares/ST/STM32_WPAN/ble/svc/Inc -I../Middlewares/ST/STM32_WPAN/ble/svc/Src -I../Utilities/sequencer -I../Middlewares/ST/STM32_WPAN/ble -I"C:/Users/Nicho/Documents/Projects/jhnvrw-seniordesign/software/WB55/BLE_SCANNER/Drivers/sx126x_driver/inc" -I"C:/Users/Nicho/Documents/Projects/jhnvrw-seniordesign/software/WB55/BLE_SCANNER/Drivers/ssd1306/inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-ssd1306-2f-src

clean-Drivers-2f-ssd1306-2f-src:
	-$(RM) ./Drivers/ssd1306/src/fonts.cyclo ./Drivers/ssd1306/src/fonts.d ./Drivers/ssd1306/src/fonts.o ./Drivers/ssd1306/src/fonts.su ./Drivers/ssd1306/src/ssd1306.cyclo ./Drivers/ssd1306/src/ssd1306.d ./Drivers/ssd1306/src/ssd1306.o ./Drivers/ssd1306/src/ssd1306.su

.PHONY: clean-Drivers-2f-ssd1306-2f-src

