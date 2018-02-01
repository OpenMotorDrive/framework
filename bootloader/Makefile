CSRC = $(shell find src -name "*.c")
INCDIR = ./include

MODULE_SEARCH_DIRS = modules
USE_PROCESS_STACKSIZE = 1280
USE_EXCEPTIONS_STACKSIZE = 256

MODULES_ENABLED = \
chibios_sys_init \
chibios_hal_init \
boot_msg \
timing \
system \
pubsub \
worker_thread \
flash \
can_driver_stm32 \
can \
can_autobaud \
uavcan \
uavcan_nodestatus_publisher \
uavcan_allocatee

MESSAGES_ENABLED = \
uavcan.protocol.GetNodeInfo \
uavcan.protocol.file.BeginFirmwareUpdate \
uavcan.protocol.file.Read \
uavcan.protocol.RestartNode

LOAD_REGION = bl

UDEFS += -DTARGET_BOOTLOADER

include ../include.mk

upload: build/$(BOARD)/bootloader.elf
	openocd -f openocd.cfg -c "program $< verify reset exit"
