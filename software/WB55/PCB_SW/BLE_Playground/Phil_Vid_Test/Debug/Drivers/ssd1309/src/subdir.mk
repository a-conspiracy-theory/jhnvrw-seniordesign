################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/ssd1309/src/driver_ssd1309.c \
../Drivers/ssd1309/src/driver_ssd1309_interface_template.c \
../Drivers/ssd1309/src/ssd1309_lib.c 

OBJS += \
./Drivers/ssd1309/src/driver_ssd1309.o \
./Drivers/ssd1309/src/driver_ssd1309_interface_template.o \
./Drivers/ssd1309/src/ssd1309_lib.o 

C_DEPS += \
./Drivers/ssd1309/src/driver_ssd1309.d \
./Drivers/ssd1309/src/driver_ssd1309_interface_template.d \
./Drivers/ssd1309/src/ssd1309_lib.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/ssd1309/src/%.o Drivers/ssd1309/src/%.su Drivers/ssd1309/src/%.cyclo: ../Drivers/ssd1309/src/%.c Drivers/ssd1309/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32WB55xx -c -I../Core/Inc -I../STM32_WPAN/App -I../Drivers/STM32WBxx_HAL_Driver/Inc -I../Drivers/STM32WBxx_HAL_Driver/Inc/Legacy -I../Utilities/lpm/tiny_lpm -I../Middlewares/ST/STM32_WPAN -I../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread -I../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl -I../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/shci -I../Middlewares/ST/STM32_WPAN/utilities -I../Middlewares/ST/STM32_WPAN/ble/core -I../Middlewares/ST/STM32_WPAN/ble/core/auto -I../Middlewares/ST/STM32_WPAN/ble/core/template -I../Middlewares/ST/STM32_WPAN/ble/svc/Inc -I../Middlewares/ST/STM32_WPAN/ble/svc/Src -I../Drivers/CMSIS/Device/ST/STM32WBxx/Include -I../Utilities/sequencer -I../Middlewares/ST/STM32_WPAN/ble -I../Drivers/CMSIS/Include -I"C:/Users/jacob/Documents/KiCad/Projects/jhnvrw-seniordesign/software/WB55/PCB_SW/BLE_Playground/Phil_Vid_Test/Drivers/ssd1309/inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-ssd1309-2f-src

clean-Drivers-2f-ssd1309-2f-src:
	-$(RM) ./Drivers/ssd1309/src/driver_ssd1309.cyclo ./Drivers/ssd1309/src/driver_ssd1309.d ./Drivers/ssd1309/src/driver_ssd1309.o ./Drivers/ssd1309/src/driver_ssd1309.su ./Drivers/ssd1309/src/driver_ssd1309_interface_template.cyclo ./Drivers/ssd1309/src/driver_ssd1309_interface_template.d ./Drivers/ssd1309/src/driver_ssd1309_interface_template.o ./Drivers/ssd1309/src/driver_ssd1309_interface_template.su ./Drivers/ssd1309/src/ssd1309_lib.cyclo ./Drivers/ssd1309/src/ssd1309_lib.d ./Drivers/ssd1309/src/ssd1309_lib.o ./Drivers/ssd1309/src/ssd1309_lib.su

.PHONY: clean-Drivers-2f-ssd1309-2f-src

