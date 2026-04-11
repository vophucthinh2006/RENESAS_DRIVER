################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/hal_entry.c \
../src/hal_warmstart.c 

C_DEPS += \
./src/hal_entry.d \
./src/hal_warmstart.d 

CREF += \
TESTING_2.cref 

OBJS += \
./src/hal_entry.o \
./src/hal_warmstart.o 

MAP += \
TESTING_2.map 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	$(file > $@.in,-mcpu=cortex-m33 -mthumb -mlittle-endian -mfloat-abi=hard -mfpu=fpv5-sp-d16 -Os -ffunction-sections -fdata-sections -fno-strict-aliasing -fmessage-length=0 -funsigned-char -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Waggregate-return -Wno-parentheses-equality -Wfloat-equal -g3 -std=c99 -fshort-enums -fno-unroll-loops -I"D:\\Renesas\\project\\TESTING\\TESTING_2\\ra_gen" -I"." -I"D:\\Renesas\\project\\TESTING\\TESTING_2\\ra_cfg\\fsp_cfg\\bsp" -I"D:\\Renesas\\project\\TESTING\\TESTING_2\\ra_cfg\\fsp_cfg" -I"D:\\Renesas\\project\\TESTING\\TESTING_2\\src" -I"D:\\Renesas\\project\\TESTING\\TESTING_2\\ra\\fsp\\inc" -I"D:\\Renesas\\project\\TESTING\\TESTING_2\\ra\\fsp\\inc\\api" -I"D:\\Renesas\\project\\TESTING\\TESTING_2\\ra\\fsp\\inc\\instances" -I"D:\\Renesas\\project\\TESTING\\TESTING_2\\ra\\arm\\CMSIS_6\\CMSIS\\Core\\Include" -D_RENESAS_RA_ -D_RA_CORE=CM33 -D_RA_ORDINAL=1 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -x c "$<" -c -o "$@")
	@clang --target=arm-none-eabi @"$@.in"

