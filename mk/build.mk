# Compiler options here.
ifeq ($(USE_OPT),)
  USE_OPT = -Os
endif

override USE_OPT += -Wl,--wrap=log10f -ggdb -std=gnu99 --specs=nosys.specs --specs=nano.specs -Werror=double-promotion -ffast-math

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

CHIBIOS ?= $(FRAMEWORK_DIR)/ChibiOS

ifeq ($(PROJECT),)
  PROJECT = $(notdir $(shell pwd))
endif

override BOARD_DIR := $(realpath $(BOARD_DIR))

BOARD := $(lastword $(subst /, ,$(BOARD_DIR)))

include $(BOARD_DIR)/board.mk

BUILDDIR = build/$(BOARD)

MODULE_SEARCH_DIRS += $(FRAMEWORK_DIR)/modules

MODULES_ENABLED_DIR := $(BUILDDIR)/modules/modules
MODULE_COPY_DIRS := $(patsubst %,$(MODULES_ENABLED_DIR)/%,$(MODULES_ENABLED))

MODULE_DIRS := $(foreach search_dir,$(MODULE_SEARCH_DIRS),$(foreach module,$(MODULES_ENABLED),$(wildcard $(search_dir)/$(module))))
MODULES_FOUND := $(notdir $(MODULE_DIRS))

define __duplicates__func
  undefine __duplicates__seen
  undefine __duplicates__result
  $$(foreach _v,$1,\
    $$(eval __duplicates__result += $$(filter $$(__duplicates__seen),$$(_v))\
    $$(eval __duplicates__seen += $$(_v))))
endef
duplicates = $(eval $(__duplicates__func))$(sort $(__duplicates__result))

ifneq ($(call duplicates, $(MODULES_FOUND)),)
  $(error Duplicated module(s): $(call duplicates, $(MODULES_FOUND)))
endif

ifneq ($(filter-out $(MODULES_FOUND), $(MODULES_ENABLED)),)
  $(error Could not find module(s): $(filter-out $(MODULES_FOUND), $(MODULES_ENABLED)))
endif

-include $(foreach module_dir,$(MODULE_DIRS),$(module_dir)/module.mk)

MODULES_CSRC := $(foreach search_dir,$(MODULE_SEARCH_DIRS),$(foreach module,$(MODULES_ENABLED),$(patsubst $(search_dir)/%,$(MODULES_ENABLED_DIR)/%,$(wildcard $(search_dir)/$(module)/*.c))))

$(foreach search_dir,$(MODULE_SEARCH_DIRS),$(foreach module,$(MODULES_ENABLED),$(foreach module_dir,$(wildcard $(search_dir)/$(module)),$(eval $(MODULES_ENABLED_DIR)/$(module): $(shell find $(module_dir))))))

MODULES_ENABLED_DEFS := $(foreach module,$(MODULES_ENABLED),-DMODULE_$(shell echo $(module) | tr a-z A-Z)_ENABLED)

COMMON_CSRC := $(shell find $(FRAMEWORK_DIR)/src -name "*.c")
COMMON_INC := $(FRAMEWORK_DIR)/include

ifneq ($(findstring stm32,$(TGT_MCU)),)
  RULESPATH = $(FRAMEWORK_DIR)/platforms/ARMCMx
  MCU  = cortex-m4
  TRGT = arm-none-eabi-
  UDEFS += -DARCH_LITTLE_ENDIAN
  ifneq ($(findstring stm32f3,$(TGT_MCU)),)
    include $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk/startup_stm32f3xx.mk
    include $(CHIBIOS)/os/hal/ports/STM32/STM32F3xx/platform.mk
  endif
  ifneq ($(findstring stm32f4,$(TGT_MCU)),)
    include $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk/startup_stm32f4xx.mk
    include $(CHIBIOS)/os/hal/ports/STM32/STM32F4xx/platform.mk
  endif
  include $(CHIBIOS)/os/common/ports/ARMCMx/compilers/GCC/mk/port_v7m.mk
endif

include $(CHIBIOS)/os/hal/hal.mk
include $(CHIBIOS)/os/hal/osal/rt/osal.mk
include $(CHIBIOS)/os/rt/rt.mk
include $(CHIBIOS)/os/hal/lib/streams/streams.mk
include $(CHIBIOS)/os/various/shell/shell.mk

INCDIR += $(CHIBIOS)/os/license \
          $(STARTUPINC) $(KERNINC) $(PORTINC) $(OSALINC) \
          $(HALINC) $(PLATFORMINC) $(BOARD_INC) $(TESTINC) $(STREAMSINC) \
          $(CHIBIOS)/community/os/various \
          $(CHIBIOS)/os/various \
          $(COMMON_INC) \
          $(BUILDDIR)/modules

INCDIR += $(SHELLINC)
# C sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CSRC += $(STARTUPSRC) \
        $(KERNSRC) \
        $(PORTSRC) \
        $(OSALSRC) \
        $(HALSRC) \
        $(PLATFORMSRC) \
        $(BOARD_SRC) \
        $(TESTSRC) \
        $(COMMON_CSRC) \
        $(MODULES_CSRC) \
        $(STREAMSSRC)

CSRC += $(SHELLSRC)

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
UDEFS += -DGIT_HASH=0x$(shell git rev-parse --short=8 HEAD) $(MODULES_ENABLED_DEFS)

# Define ASM defines here
UADEFS =

# List all user directories here
UINCDIR =

# List the user directory to look for the libraries here
ULIBDIR =

# List all user libraries here
ULIBS = -lm

# Select region for application
LOAD_REGION ?= app

LDSCRIPT = $(RULESPATH)/ld/$(TGT_MCU)/$(LOAD_REGION).ld

include $(RULESPATH)/rules.mk

$(MODULE_COPY_DIRS):
	rm -rf $@
	mkdir -p $(dir $@)
	cp -R -p $(wildcard $(addsuffix /$(patsubst $(MODULES_ENABLED_DIR)/%,%,$@),$(MODULE_SEARCH_DIRS))) $@

$(CSRC): $(MODULE_COPY_DIRS)
