################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/ssd1306/src/fonts.c \
../Drivers/ssd1306/src/ssd1306.c \
../Drivers/ssd1306/src/ssd1306_lib.c \
../Drivers/ssd1306/src/ui.c 

OBJS += \
./Drivers/ssd1306/src/fonts.o \
./Drivers/ssd1306/src/ssd1306.o \
./Drivers/ssd1306/src/ssd1306_lib.o \
./Drivers/ssd1306/src/ui.o 

C_DEPS += \
./Drivers/ssd1306/src/fonts.d \
./Drivers/ssd1306/src/ssd1306.d \
./Drivers/ssd1306/src/ssd1306_lib.d \
./Drivers/ssd1306/src/ui.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/ssd1306/src/%.o Drivers/ssd1306/src/%.su Drivers/ssd1306/src/%.cyclo: ../Drivers/ssd1306/src/%.c Drivers/ssd1306/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32WB55xx -c -I../Core/Inc -I../Drivers/sx126x_driver/inc -I../STM32_WPAN/App -I../Drivers/STM32WBxx_HAL_Driver/Inc -I../Drivers/STM32WBxx_HAL_Driver/Inc/Legacy -I../Utilities/lpm/tiny_lpm -I../Middlewares/ST/STM32_WPAN -I../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread -I../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl -I../Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/shci -I../Middlewares/ST/STM32_WPAN/utilities -I../Middlewares/ST/STM32_WPAN/ble/core -I../Middlewares/ST/STM32_WPAN/ble/core/auto -I../Middlewares/ST/STM32_WPAN/ble/core/template -I../Middlewares/ST/STM32_WPAN/ble/svc/Inc -I../Middlewares/ST/STM32_WPAN/ble/svc/Src -I../Drivers/CMSIS/Device/ST/STM32WBxx/Include -I../Middlewares/ST/STM32_WPAN/ble -I../Drivers/CMSIS/Include -I"C:/Users/jacob/Desktop/Handheld/Phil_Vid_Test_V9_Full/Drivers/ssd1306/inc" -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Drivers/CMSIS/RTOS2/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-ssd1306-2f-src

clean-Drivers-2f-ssd1306-2f-src:
	-$(RM) ./Drivers/ssd1306/src/fonts.cyclo ./Drivers/ssd1306/src/fonts.d ./Drivers/ssd1306/src/fonts.o ./Drivers/ssd1306/src/fonts.su ./Drivers/ssd1306/src/ssd1306.cyclo ./Drivers/ssd1306/src/ssd1306.d ./Drivers/ssd1306/src/ssd1306.o ./Drivers/ssd1306/src/ssd1306.su ./Drivers/ssd1306/src/ssd1306_lib.cyclo ./Drivers/ssd1306/src/ssd1306_lib.d ./Drivers/ssd1306/src/ssd1306_lib.o ./Drivers/ssd1306/src/ssd1306_lib.su ./Drivers/ssd1306/src/ui.cyclo ./Drivers/ssd1306/src/ui.d ./Drivers/ssd1306/src/ui.o ./Drivers/ssd1306/src/ui.su

.PHONY: clean-Drivers-2f-ssd1306-2f-src

