openmotordrive/framework is a framework for embedded applications, primarily targeted at drone accessories that connect via CAN bus and the UAVCAN protocol.

## Feature set
- RTOS-based (ChibiOS)
- Minimizes boilerplate code in application
- Modular architecture
- Supports STM32F3 targets, ready to support many others
- Includes startup code and ld scripts for supported MCUs
- UAVCAN bootloader provided
- Makefile-based build system provided
- Flash-based configuration parameter store
- UAVCAN support, including auto-bauding, dynamic node id allocation, parameter interface, firmware update
- Internal publish-subscribe messaging system
- Worker threads with API for creating tasks that run on an interval and tasks that handle pubsub messages
- HAL for SPI devices
- HAL for I2C devices
- Includes some device drivers

## Included modules
|Name|Description|
|---|---|
|app_descriptor|Provides app descriptor in flash, which is used by openmotordrive/bootloader to identify a valid application|
|boot_msg|Provides support for boot messages in SRAM, which are used to pass messages from bootloader->app and app->bootloader|
|can|Wraps ChibiOS CAN driver|
|can_auto_init|Uses constructor functions to initialize CAN bus. Obtains baud rate setting from boot message, app descriptor, or performs auto baud detection|
|chibios_hal_init|Uses constructor functions to initialize ChibiOS HAL|
|chibios_sys_init|Uses constructor functions to initialize ChibiOS|
|dw1000|Driver for DecaWave DW1000|
|flash|Provides HAL for flash write and erase|
|lpwork_thread|Provides a standard worker thread for low-priority tasks|
|param|Provides flash parameter support|
|profiLED|Driver for 2-wire SPI LEDs|
|pubsub|Provides internal publish-subscribe messaging|
|spi_device|Provides spi device abstraction|
|system|Provides misc system functions, e.g. reboot|
|timing|Provides Arduino-compatible millis(), micros() functions|
|uavcan|Provides uavcan transmit and receive functionality, includes uavcan standard message definitions|
|uavcan_allocatee|Provides dynamic node id allocation|
|uavcan_beginfirmwareupdate_server|Provides a uavcan.protocol.file.BeginFirmwareUpdate server. Uses boot_msg to command bootloader to enter software update mode|
|uavcan_getnodeinfo_server|Provides a uavcan.protocol.GetNodeInfo server|
|uavcan_nodestatus_publisher|Provides a uavcan.protocol.NodeStatus publisher|
|uavcan_param_interface|Provides a uavcan interface for param|
|uavcan_restart|Provides a uavcan.protocol.RestartNode server|
|worker_thread|Provides worker threads that can process timer tasks, which run after a delay, or listener tasks, which listen to pubsub messages|

## REQUIREMENTS
Some installations requirements and help.

### Submodules :
After cloning, get the submodules with :

```bash
git submodule update --init --recursive
```

### Toolchain :
Tested with gcc6 and gcc7

On Ubuntu 16.04, you can use [team-gcc-arm-embedded](https://launchpad.net/~team-gcc-arm-embedded/+archive/ubuntu/ppa) version

### Others
It needs crcmod and uavcan python package

```bash
sudo pip install -U crcmod uavcan
```

