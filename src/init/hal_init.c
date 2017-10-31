#include <common/ctor.h>

#include <hal.h>

RUN_ON(CH_HAL_INIT) {
    halInit();
}
