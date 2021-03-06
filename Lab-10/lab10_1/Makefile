CROSS_COMPILE=arm-none-eabi-
QEMU_STM32 ?= ../qemu_stm32/arm-softmmu/qemu-system-arm

ARCH=CM3
VENDOR=ST
PLAT=STM32F10x

LIBDIR = ../../Lab-8/freertos
CMSIS_LIB=$(LIBDIR)/libraries/CMSIS/$(ARCH)
STM32_LIB=$(LIBDIR)/libraries/STM32F10x_StdPeriph_Driver

CMSIS_PLAT_SRC = $(CMSIS_LIB)/DeviceSupport/$(VENDOR)/$(PLAT)

FREERTOS_SRC = $(LIBDIR)/libraries/FreeRTOS
FREERTOS_INC = $(FREERTOS_SRC)/include/                                       
FREERTOS_PORT_INC = $(FREERTOS_SRC)/portable/GCC/ARM_$(ARCH)/

all: main.bin

main.bin: main.c string-util.c stm32_p103.c \
		$(FREERTOS_SRC)/tasks.c \
		$(FREERTOS_SRC)/croutine.c \
		$(FREERTOS_SRC)/queue.c \
		$(FREERTOS_SRC)/list.c
	$(CROSS_COMPILE)gcc \
		-std=gnu99 \
		-Wl,-Tmain.ld -nostartfiles \
		-I. -I$(FREERTOS_INC) -I$(FREERTOS_PORT_INC) \
		-I$(LIBDIR)/libraries/CMSIS/CM3/CoreSupport \
		-I$(LIBDIR)/libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x \
		-I$(LIBDIR)/libraries/STM32F10x_StdPeriph_Driver/inc \
		-fno-common -O0 \
		-gdwarf-2 -g3 \
		-mcpu=cortex-m3 -mthumb \
		-o main.elf \
		\
		$(CMSIS_LIB)/CoreSupport/core_cm3.c \
		$(CMSIS_PLAT_SRC)/system_stm32f10x.c \
		$(CMSIS_PLAT_SRC)/startup/gcc_ride7/startup_stm32f10x_md.s \
		$(STM32_LIB)/src/stm32f10x_rcc.c \
		$(STM32_LIB)/src/stm32f10x_gpio.c \
		$(STM32_LIB)/src/stm32f10x_usart.c \
		$(STM32_LIB)/src/stm32f10x_exti.c \
		$(STM32_LIB)/src/misc.c \
		\
		$(FREERTOS_SRC)/croutine.c \
		$(FREERTOS_SRC)/list.c \
		$(FREERTOS_SRC)/queue.c \
		$(FREERTOS_SRC)/tasks.c \
		$(FREERTOS_SRC)/portable/GCC/ARM_CM3/port.c \
		$(FREERTOS_SRC)/portable/MemMang/heap_1.c \
		\
		stm32_p103.c \
		string-util.c \
		main.c
	$(CROSS_COMPILE)objcopy -Obinary main.elf main.bin
	$(CROSS_COMPILE)objdump -S main.elf > main.list

qemu: main.bin $(QEMU_STM32)
	$(QEMU_STM32) -M stm32-p103 -kernel main.bin

qemudbg: main.bin $(QEMU_STM32)
	$(QEMU_STM32) -M stm32-p103 \
		-gdb tcp::3333 -S \
		-kernel main.bin
qemuauto: main.bin $(QEMU_STM32)
	$(QEMU_STM32) -M stm32-p103 \
		-monitor stdio\
		-gdb tcp::3333 -S \
		-kernel main.bin & echo "$$!" > "qemu.pid"
	arm-none-eabi-gdb -x test.script
	kill -9 `cat qemu.pid`
	rm -f qemu.pid
	python gdb2vcd.py
	gtkwave sched.vcd

emu: main.bin
	bash emulate.sh main.bin

clean:
	rm -f *.elf *.bin *.list
