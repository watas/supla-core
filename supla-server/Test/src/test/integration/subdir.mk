################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/test/integration/IntegrationTest.cpp \
../src/test/integration/MySqlShell.cpp 

CPP_DEPS += \
./src/test/integration/IntegrationTest.d \
./src/test/integration/MySqlShell.d 

OBJS += \
./src/test/integration/IntegrationTest.o \
./src/test/integration/MySqlShell.o 


# Each subdirectory must supply rules for building sources it contributes
src/test/integration/%.o: ../src/test/integration/%.cpp src/test/integration/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -D__DEBUG=1 -DUSE_DEPRECATED_EMEV_V1 -D__TEST=1 -D__OPENSSL_TOOLS=1 -D__BCRYPT=1 -I../src -I../src/asynctask -I../src/mqtt -I$(INCMYSQL) -I../src/user -I../src/device -I../src/client -I$(SSLDIR)/include -I../src/test -O2 -g3 -Wall -fsigned-char -c -fmessage-length=0 -fstack-protector-all -D_FORTIFY_SOURCE=2 -std=c++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src-2f-test-2f-integration

clean-src-2f-test-2f-integration:
	-$(RM) ./src/test/integration/IntegrationTest.d ./src/test/integration/IntegrationTest.o ./src/test/integration/MySqlShell.d ./src/test/integration/MySqlShell.o

.PHONY: clean-src-2f-test-2f-integration

