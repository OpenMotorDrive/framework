# Compiler options here.
ifeq ($(USE_OPT),)
  USE_OPT = -Os -ggdb --specs=nosys.specs -lnosys -lm -ffast-math
endif

# C specific options here (added to USE_OPT).
ifeq ($(USE_COPT),)
  USE_COPT =
endif

# C++ specific options here (added to USE_OPT).
ifeq ($(USE_CPPOPT),)
  USE_CPPOPT = -fno-rtti
endif

# Enable this if you want the linker to remove unused code and data
ifeq ($(USE_LINK_GC),)
  USE_LINK_GC = yes
endif

# Linker extra options here.
ifeq ($(USE_LDOPT),)
  USE_LDOPT =
endif

# Enable this if you want link time optimizations (LTO)
ifeq ($(USE_LTO),)
  USE_LTO = yes
endif

# If enabled, this option allows to compile the application in THUMB mode.
ifeq ($(USE_THUMB),)
  USE_THUMB = yes
endif

# Enable this if you want to see the full log while compiling.
ifeq ($(USE_VERBOSE_COMPILE),)
  USE_VERBOSE_COMPILE = no
endif

# If enabled, this option makes the build process faster by not compiling
# modules not used in the current configuration.
ifeq ($(USE_SMART_BUILD),)
  USE_SMART_BUILD = no
endif

ifeq ($(USE_PROCESS_STACKSIZE),)
  USE_PROCESS_STACKSIZE = 0x500
endif

ifeq ($(USE_EXCEPTIONS_STACKSIZE),)
  USE_EXCEPTIONS_STACKSIZE = 0x500
endif

ifeq ($(USE_FPU),)
  USE_FPU = hard
endif

OMD_COMMON_DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))

CHIBIOS = $(OMD_COMMON_DIR)/ChibiOS_17.6.0

CANARD_DIR = $(OMD_COMMON_DIR)/libcanard

ifeq ($(PROJECT),)
  PROJECT = $(notdir $(shell pwd))
endif

ifneq ($(BOARD),)
  include boards/$(BOARD)/board.mk
endif

BUILDDIR = build/$(BOARD)

ifneq ($(findstring stm32,$(TGT_MCU)),)
  RULESPATH = $(OMD_COMMON_DIR)/rules/ARMCMx
  MCU  = cortex-m4
  TRGT = arm-none-eabi-
  UDEFS += -DARCH_LITTLE_ENDIAN
  ifneq ($(findstring stm32f3,$(TGT_MCU)),)
    include $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk/startup_stm32f3xx.mk
    include $(CHIBIOS)/os/hal/ports/STM32/STM32F3xx/platform.mk
  endif
  include $(CHIBIOS)/os/common/ports/ARMCMx/compilers/GCC/mk/port_v7m.mk
endif

include $(CHIBIOS)/os/hal/hal.mk
include $(CHIBIOS)/os/hal/osal/rt/osal.mk
include $(CHIBIOS)/os/rt/rt.mk

COMMON_CSRC = $(shell find $(OMD_COMMON_DIR)/src -name "*.c") $(CANARD_DIR)/canard.c
COMMON_INC = $(OMD_COMMON_DIR)/include $(CANARD_DIR)

INCDIR += $(CHIBIOS)/os/license \
          $(STARTUPINC) $(KERNINC) $(PORTINC) $(OSALINC) \
          $(HALINC) $(PLATFORMINC) $(BOARDINC) $(TESTINC) \
          $(CHIBIOS)/community/os/various \
          $(CHIBIOS)/os/various \
          $(COMMON_INC)

# C sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CSRC += $(STARTUPSRC) \
        $(KERNSRC) \
        $(PORTSRC) \
        $(OSALSRC) \
        $(HALSRC) \
        $(PLATFORMSRC) \
        $(BOARDSRC) \
        $(TESTSRC) \
        $(COMMON_CSRC)

# C++ sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CPPSRC +=

# C sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
ACSRC +=

# C++ sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
ACPPSRC +=

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
TCSRC +=

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
TCPPSRC +=

# List ASM source files here
ASMSRC +=
ASMXSRC += $(STARTUPASM) $(PORTASM) $(OSALASM)

CC   = $(TRGT)gcc
CPPC = $(TRGT)g++
# Enable loading with g++ only if you need C++ runtime support.
# NOTE: You can use C++ even without C++ support if you are careful. C++
#       runtime support makes code size explode.
LD   = $(TRGT)gcc
#LD   = $(TRGT)g++
CP   = $(TRGT)objcopy
AS   = $(TRGT)gcc -x assembler-with-cpp
AR   = $(TRGT)ar
OD   = $(TRGT)objdump
SZ   = $(TRGT)size
HEX  = $(CP) -O ihex
BIN  = $(CP) -O binary

# ARM-specific options here
AOPT =

# THUMB-specific options here
TOPT = -mthumb -DTHUMB

# Define C warning options here
CWARN = -Wall -Wextra -Wundef -Wstrict-prototypes

# Define C++ warning options here
CPPWARN = -Wall -Wextra -Wundef

# List all user C define here, like -D_DEBUG=1
UDEFS += -DGIT_HASH=0x$(shell git rev-parse --short=8 HEAD) -D"CANARD_ASSERT(x)"="{}"

# Define ASM defines here
UADEFS =

# List all user directories here
UINCDIR =

# List the user directory to look for the libraries here
ULIBDIR =

# List all user libraries here
ULIBS =

LDSCRIPT = $(RULESPATH)/ld/$(TGT_MCU)/app.ld
include $(RULESPATH)/rules.mk

POST_MAKE_ALL_RULE_HOOK: $(BUILDDIR)/$(PROJECT).bin
	python $(OMD_COMMON_DIR)/tools/crc_binary.py $(BUILDDIR)/$(PROJECT).bin $(BUILDDIR)/$(PROJECT).bin
