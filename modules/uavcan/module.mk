UAVCAN_MODULE_DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))

CSRC += $(UAVCAN_MODULE_DIR)/libcanard/canard.c

UDEFS += -D"CANARD_ASSERT(x)"="{}"

DSDL_NAMESPACE_DIR_UAVCAN ?= $(FRAMEWORK_DIR)/dsdl/uavcan

DSDL_NAMESPACE_DIRS += $(DSDL_NAMESPACE_DIR_UAVCAN) $(VENDOR_DSDL_NAMESPACE_DIRS)

INCDIR += $(BUILDDIR)/dsdlc/include

$(BUILDDIR)/dsdlc.mk: $(foreach dsdl_dir,$(wildcard $(DSDL_NAMESPACE_DIRS)),$(shell find $(dsdl_dir)))
	rm -rf $(BUILDDIR)/dsdlc
	python $(UAVCAN_MODULE_DIR)/canard_dsdlc/canard_dsdlc.py $(addprefix --build=,$(MESSAGES_ENABLED)) $(DSDL_NAMESPACE_DIRS) $(BUILDDIR)/dsdlc
	find $(BUILDDIR)/dsdlc/src -name "*.c" | xargs echo CSRC += > $(BUILDDIR)/dsdlc.mk

ifneq ($(MAKECMDGOALS),clean)
-include $(BUILDDIR)/dsdlc.mk
endif
