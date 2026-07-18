################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/sx126x_driver/src/lr_fhss_mac.c \
../Drivers/sx126x_driver/src/sx126x.c \
../Drivers/sx126x_driver/src/sx126x_bpsk.c \
../Drivers/sx126x_driver/src/sx126x_driver_version.c \
../Drivers/sx126x_driver/src/sx126x_lr_fhss.c 

OBJS += \
./Drivers/sx126x_driver/src/lr_fhss_mac.o \
./Drivers/sx126x_driver/src/sx126x.o \
./Drivers/sx126x_driver/src/sx126x_bpsk.o \
./Drivers/sx126x_driver/src/sx126x_driver_version.o \
./Drivers/sx126x_driver/src/sx126x_lr_fhss.o 

C_DEPS += \
./Drivers/sx126x_driver/src/lr_fhss_mac.d \
./Drivers/sx126x_driver/src/sx126x.d \
./Drivers/sx126x_driver/src/sx126x_bpsk.d \
./Drivers/sx126x_driver/src/sx126x_driver_version.d \
./Drivers/sx126x_driver/src/sx126x_lr_fhss.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/sx126x_driver/src/%.o Drivers/sx126x_driver/src/%.su Drivers/sx126x_driver/src/%.cyclo: ../Drivers/sx126x_driver/src/%.c Drivers/sx126x_driver/src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_NUCLEO_64 -DUSE_HAL_DRIVER -DSTM32WB55xx -c -I"C:/Users/jacob/Documents/STM32/Nucleo-WB55-Board/Drivers/sx126x_driver/inc" -I../Core/Inc -I../Drivers/STM32WBxx_HAL_Driver/Inc -I../Drivers/STM32WBxx_HAL_Driver/Inc/Legacy -I../Drivers/BSP/P-NUCLEO-WB55.Nucleo -I../Drivers/CMSIS/Device/ST/STM32WBxx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-sx126x_driver-2f-src

clean-Drivers-2f-sx126x_driver-2f-src:
	-$(RM) ./Drivers/sx126x_driver/src/lr_fhss_mac.cyclo ./Drivers/sx126x_driver/src/lr_fhss_mac.d ./Drivers/sx126x_driver/src/lr_fhss_mac.o ./Drivers/sx126x_driver/src/lr_fhss_mac.su ./Drivers/sx126x_driver/src/sx126x.cyclo ./Drivers/sx126x_driver/src/sx126x.d ./Drivers/sx126x_driver/src/sx126x.o ./Drivers/sx126x_driver/src/sx126x.su ./Drivers/sx126x_driver/src/sx126x_bpsk.cyclo ./Drivers/sx126x_driver/src/sx126x_bpsk.d ./Drivers/sx126x_driver/src/sx126x_bpsk.o ./Drivers/sx126x_driver/src/sx126x_bpsk.su ./Drivers/sx126x_driver/src/sx126x_driver_version.cyclo ./Drivers/sx126x_driver/src/sx126x_driver_version.d ./Drivers/sx126x_driver/src/sx126x_driver_version.o ./Drivers/sx126x_driver/src/sx126x_driver_version.su ./Drivers/sx126x_driver/src/sx126x_lr_fhss.cyclo ./Drivers/sx126x_driver/src/sx126x_lr_fhss.d ./Drivers/sx126x_driver/src/sx126x_lr_fhss.o ./Drivers/sx126x_driver/src/sx126x_lr_fhss.su

.PHONY: clean-Drivers-2f-sx126x_driver-2f-src

