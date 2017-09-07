ifeq ($(USE_SMART_BUILD),yes)
$(info USE_SMART_BUILD yes)

ifneq ($(findstring HAL_USE_PAL TRUE,$(HALCONF)),)
$(info HAL_USE_PAL TRUE)
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/STM32/LLD/GPIOv2/hal_pal_lld.c
endif
else
$(info USE_SMART_BUILD FALSE)
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/STM32/LLD/GPIOv2/hal_pal_lld.c
endif

PLATFORMINC += $(CHIBIOS)/os/hal/ports/STM32/LLD/GPIOv2
