CSRC = $(shell find src -name "*.c")
INCDIR = ./include
USE_OPT = -O1 -g
USE_LTO = no
MODULES_ENABLED = \
chibios_sys_init \
chibios_hal_init \
app_descriptor \
boot_msg \
timing \
system \
pubsub \
worker_thread \
can_driver_stm32 \
can \
can_autobaud \
uavcan \
uavcan_nodestatus_publisher \
uavcan_getnodeinfo_server \
uavcan_beginfirmwareupdate_server \
uavcan_allocatee \
uavcan_restart \
freemem_check \
spi_device \
uavcan_debug \
driver_ak09916 \
driver_icm20x48

MESSAGES_ENABLED = \
uavcan.protocol.debug.LogMessage

include ../../include.mk
