BOARD ?= org.proficnc.uwb_1.0

CSRC = $(shell find src -name "*.c")
INCDIR = ./include

MODULES_ENABLED = \
chibios_sys_init \
chibios_hal_init \
app_descriptor \
boot_msg \
timing system \
pubsub \
flash \
param \
worker_thread \
lpwork_thread \
can \
can_auto_init \
uavcan \
uavcan_nodestatus_publisher \
uavcan_getnodeinfo_server \
uavcan_beginfirmwareupdate_server \
uavcan_debug \
uavcan_param_interface \
uavcan_allocatee \
uavcan_restart

MESSAGES_ENABLED = \
uavcan.protocol.debug.LogMessage

include ../../include.mk
