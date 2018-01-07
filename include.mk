FRAMEWORK_DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))

all:

ifeq ($(filter clean,$(MAKECMDGOALS)),)
  include $(FRAMEWORK_DIR)/mk/build.mk
endif

clean:
	rm -rf build
