BOARD ?= org.proficnc.uwb_1.0

CSRC = $(shell find src -name "*.c")
INCDIR = ./include

MODULES_ENABLED = \
chibios_sys_init \
chibios_hal_init \
chibios_debug \
app_descriptor \
boot_msg \
timing \
system \
pubsub \
flash \
param \
worker_thread \
can \
can_driver_stm32 \
can_autobaud \
uavcan \
uavcan_nodestatus_publisher \
uavcan_getnodeinfo_server \
uavcan_allocatee

MESSAGES_ENABLED = \
uavcan.protocol.debug.LogMessage

include ../../include.mk
