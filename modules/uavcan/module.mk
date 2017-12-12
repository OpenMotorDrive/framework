UAVCAN_MODULE_DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))

CSRC += $(UAVCAN_MODULE_DIR)/libcanard/canard.c

UDEFS += -D"CANARD_ASSERT(x)"="{}"

DSDL_NAMESPACE_DIR_UAVCAN ?= $(FRAMEWORK_DIR)/dsdl/uavcan

DSDL_NAMESPACE_DIRS += $(DSDL_NAMESPACE_DIR_UAVCAN)

INCDIR += $(BUILDDIR)/dsdlc/include

$(BUILDDIR)/dsdlc.mk: $(shell find $(DSDL_NAMESPACE_DIRS)) $(shell find $(VENDOR_DSDL_NAMESPACE_DIRS))
	rm -rf $(BUILDDIR)/dsdlc
	python $(UAVCAN_MODULE_DIR)/canard_dsdlc/canard_dsdlc.py $(addprefix --build=,$(MESSAGES_ENABLED)) $(DSDL_NAMESPACE_DIRS) $(VENDOR_DSDL_NAMESPACE_DIRS) $(BUILDDIR)/dsdlc
	find $(BUILDDIR)/dsdlc/src -name "*.c" | xargs echo CSRC += > $(BUILDDIR)/dsdlc.mk

ifneq ($(MAKECMDGOALS),clean)
-include $(BUILDDIR)/dsdlc.mk
endif
