OMD_Framework is a framework for embedded applications, primarily targeted at drone accessories that connect via CAN bus and the UAVCAN protocol. OMD_Framework is under development, and has not yet achieved its targeted minimum feature set.

## Targeted minimum feature set
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

## Initial modules
|Name|Description|Dependencies|
|---|---|---|
|worker_thread|Provides worker threads that can perform tasks on an interval, or upon receiving a pubsub message||
|lpwork_thread|Provides a standard worker thread for low-priority tasks|worker_thread|
|chibios_sys_init|Uses constructor functions to initialize ChibiOS||
|chibios_hal_init|Uses constructor functions to initialize ChibiOS HAL||
|bootloader_compat|runs during init, retreives boot message left by bootloader, provides app descriptor in flash||
|timing|Arduino-compatible millis(), micros() functions|lpwork_thread|
|pubsub|Internal publish-subscribe messaging||
|can|Starts ChibiOS CAN driver, retrieves can bus baud rate from bootloader_compat or auto-bauds if required||
|uavcan|Provides uavcan transmit and receive functionality, includes uavcan standard message definitions|pubsub|
|uavcan_allocatee|provides dynamic node id allocation|uavcan|
|uavcan_basic_node|provides NodeStatus publisher and GetNodeInfo server, retrieves node info from bootloader_compat|uavcan, lpwork_thread|
|flash|provides HAL for flash write and erase||
|param|provides flash parameter support|flash|
|param_interface_uavcan|provides uavcan interface for param|uavcan, param|
|param_change_notifier|publishes notifications when params change|pubsub, param|
|spi_device|provides spi device abstraction||
|profiLED|driver for 2-wire SPI LEDs|spi_device, lpwork_thread|
|dw1000|driver for DecaWave DW1000|spi_device|
