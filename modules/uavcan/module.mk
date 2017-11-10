MODULE_DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))
MODULES_CSRC += $(MODULE_DIR)/libcanard/canard.c

UDEFS += -D"CANARD_ASSERT(x)"="{}"

DSDL_NAMESPACE_DIR_UAVCAN ?= $(FRAMEWORK_DIR)/dsdl/uavcan

DSDL_NAMESPACE_DIRS += $(DSDL_NAMESPACE_DIR_UAVCAN)

INCDIR += $(BUILDDIR)/dsdlc/include

ifneq ($(MAKECMDGOALS),clean)
include $(BUILDDIR)/dsdlc.mk
endif

$(BUILDDIR)/dsdlc.mk:
	rm -rf $(BUILDDIR)/dsdlc
	python $(MODULE_DIR)/canard_dsdlc/canard_dsdlc.py $(addprefix --build=,$(MESSAGES_ENABLED)) $(DSDL_NAMESPACE_DIRS) $(BUILDDIR)/dsdlc
	find $(BUILDDIR)/dsdlc/src -name "*.c" | xargs echo CSRC += > $@
