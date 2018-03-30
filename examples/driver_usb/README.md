## Usage
Set BOARD_DIR according to you board directory.

### Example

```bash
BOARD_DIR=boards/com.hex.cube_1.0 make
```
init the gpio used in board init , the auto initialisation from chibios as been removed to save flash

Add a new board type :
Add new platform in modules/platform_YOURBOARDNAME
Add new platform in platforms/ARMCMx/ld/YOURBOARDNAME

modify :
 - boards/XXX/mcuconf.h
 - boards/XXX/board.mk
 - boards/XXX/board.h


Modify Makefile for include etc.
program file to compile go to src.

Modify framework_conf.h for framework or chibios configuration
Modify MCU config in boards / XXXX/ mcuconf.h


openocd :
config 
source [find interface/stlink-v2-1.cfg]

transport select hla_swd

source [find target/stm32f4x.cfg]

reset_config srst_only
// end config

reset halt

flash write_image erase /home/pierre/Workspace/framework/examples/driver_usb/build/com.skt.usb_1.0/driver_usb.elf