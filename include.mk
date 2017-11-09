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

FRAMEWORK_DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))

CHIBIOS = $(FRAMEWORK_DIR)/ChibiOS_17.6.0

CANARD_DIR = $(FRAMEWORK_DIR)/libcanard

ifeq ($(PROJECT),)
  PROJECT = $(notdir $(shell pwd))
endif

ifneq ($(BOARD),)
  include boards/$(BOARD)/board.mk
endif

BUILDDIR = build/$(BOARD)

ifneq ($(findstring stm32,$(TGT_MCU)),)
  RULESPATH = $(FRAMEWORK_DIR)/rules/ARMCMx
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

MODULE_SEARCH_DIRS := $(FRAMEWORK_DIR)/modules modules

COMMON_MODULE_DIRS := $(foreach module,$(MODULES_ENABLED),$(wildcard $(FRAMEWORK_DIR)/modules/$(module)))
COMMON_MODULES := $(patsubst $(FRAMEWORK_DIR)/modules/%,%,$(COMMON_MODULE_DIRS))

APP_MODULE_DIRS := $(foreach module,$(MODULES_ENABLED),$(wildcard modules/$(module)))
APP_MODULES := $(patsubst modules/%,%,$(APP_MODULE_DIRS))

ifneq ($(filter $(COMMON_MODULES), $(APP_MODULES)),)
  $(error Duplicated module(s): $(filter $(COMMON_MODULES), $(APP_MODULES)))
endif

MODULES_FOUND := $(COMMON_MODULES) $(APP_MODULES)
ifneq ($(filter-out $(MODULES_FOUND), $(MODULES_ENABLED)),)
  $(error Could not find module(s): $(filter-out $(MODULES_FOUND), $(MODULES_ENABLED)))
endif

MODULE_DIRS := $(COMMON_MODULE_DIRS) $(APP_MODULE_DIRS)

-include $(foreach module_dir,$(MODULE_DIRS),$(module_dir)/module.mk)
MODULES_CSRC += $(foreach module_dir,$(MODULE_DIRS),$(shell find $(module_dir) -name "*.c"))
MODULES_INC += $(foreach module_dir,$(MODULE_DIRS),$(wildcard $(module_dir)/include))

MODULES_ENABLED_DEFS := $(foreach module,$(MODULES_ENABLED),-DMODULE_$(shell echo $(module) | tr a-z A-Z)_ENABLED)

COMMON_CSRC := $(shell find $(FRAMEWORK_DIR)/src -name "*.c") $(CANARD_DIR)/canard.c
COMMON_INC := $(FRAMEWORK_DIR)/include $(CANARD_DIR)

INCDIR += $(CHIBIOS)/os/license \
          $(STARTUPINC) $(KERNINC) $(PORTINC) $(OSALINC) \
          $(HALINC) $(PLATFORMINC) $(BOARDINC) $(TESTINC) \
          $(CHIBIOS)/community/os/various \
          $(CHIBIOS)/os/various \
          $(COMMON_INC) \
          $(BUILDDIR)/module_includes \
          $(BUILDDIR)/dsdlc/include

# C sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
ifneq ($(MAKECMDGOALS),clean)
include $(BUILDDIR)/dsdlc.mk
endif
CSRC += $(STARTUPSRC) \
        $(KERNSRC) \
        $(PORTSRC) \
        $(OSALSRC) \
        $(HALSRC) \
        $(PLATFORMSRC) \
        $(BOARDSRC) \
        $(TESTSRC) \
        $(COMMON_CSRC) \
        $(MODULES_CSRC)
#         $(shell find $(BUILDDIR)/dsdlc/src -name "*.c")

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
UDEFS += -DGIT_HASH=0x$(shell git rev-parse --short=8 HEAD) -D"CANARD_ASSERT(x)"="{}" $(MODULES_ENABLED_DEFS)

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

DSDL_NAMESPACE_DIRS += $(FRAMEWORK_DIR)/dsdl/uavcan

$(BUILDDIR)/dsdlc.mk:
	rm -rf $(BUILDDIR)/dsdlc
	python $(FRAMEWORK_DIR)/tools/canard_dsdlc/canard_dsdlc.py $(addprefix --build=,$(MESSAGES_ENABLED)) $(DSDL_NAMESPACE_DIRS) $(BUILDDIR)/dsdlc
	find $(BUILDDIR)/dsdlc/src -name "*.c" | xargs echo CSRC += > $@

MODULES_INC_COPIES := $(foreach module,$(MODULES_ENABLED),$(BUILDDIR)/module_includes/$(module))
$(MODULES_INC_COPIES):
	mkdir -p $(dir $@)
	cp -R $(wildcard $(addsuffix /$(patsubst $(BUILDDIR)/module_includes/%,%,$@),$(MODULE_SEARCH_DIRS))) $@
PRE_BUILD_RULE: $(MODULES_INC_COPIES)

POST_MAKE_ALL_RULE_HOOK: $(BUILDDIR)/$(PROJECT).bin
	python $(FRAMEWORK_DIR)/tools/crc_binary.py $(BUILDDIR)/$(PROJECT).bin $(BUILDDIR)/$(PROJECT).bin

.PHONY: PRE_BUILD_RULE
PRE_BUILD_RULE:
	cd $(FRAMEWORK_DIR) && git submodule init && git submodule update

# This ensures that PRE_BUILD_RULE is executed first and non-concurrently
ifneq ($(MAKECMDGOALS),clean)
-include PRE_BUILD_RULE
endif
