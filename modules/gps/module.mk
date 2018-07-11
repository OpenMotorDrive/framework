GPS_MODULE_DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))

INCDIR += $(BUILDDIR)/ubx_msgs/include

$(BUILDDIR)/ubx_msgs.mk: $(GPS_MODULE_DIR)/ubx_parser
	rm -rf $(BUILDDIR)/ubx_msgs
	python $(GPS_MODULE_DIR)/ubx_parser/ubx_pdf_csv_parser.py $(addprefix --build=,$(UBX_MESSAGES_ENABLED)) $(BUILDDIR)/ubx_msgs 
	find $(BUILDDIR)/ubx_msgs/src -name "*.c" | xargs echo CSRC += > $(BUILDDIR)/ubx_msgs.mk

ifneq ($(MAKECMDGOALS),clean)
-include $(BUILDDIR)/ubx_msgs.mk
endif
