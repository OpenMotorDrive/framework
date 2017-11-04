#include <common/ctor.h>

#include <ch.h>

RUN_ON(CH_SYS_INIT) {
    chSysInit();
}
