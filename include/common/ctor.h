#pragma once

#define __CTOR_CONCAT(a,b) a ## b
#define _CTOR_CONCAT(a,b) __CTOR_CONCAT(a,b)

#define RUN_ON(SEQ_NAME) \
static void __attribute__((unused,constructor(100+3*(_CTOR_SEQUENCE_LENGTH+_CTOR_CONCAT(_CTOR_SEQUENCE_,SEQ_NAME))))) _CTOR_CONCAT(_local_ctor_,__LINE__)(void)
#define RUN_BEFORE(SEQ_NAME) \
static void __attribute__((unused,constructor(100+3*(_CTOR_SEQUENCE_LENGTH+_CTOR_CONCAT(_CTOR_SEQUENCE_,SEQ_NAME))-1))) _CTOR_CONCAT(_local_ctor_,__LINE__)(void)
#define RUN_AFTER(SEQ_NAME) \
static void __attribute__((unused,constructor(100+3*(_CTOR_SEQUENCE_LENGTH+_CTOR_CONCAT(_CTOR_SEQUENCE_,SEQ_NAME))+1))) _CTOR_CONCAT(_local_ctor_,__LINE__)(void)

enum {
    _CTOR_SEQUENCE_BOOT_MSG_RETRIEVAL,
    _CTOR_SEQUENCE_CH_HAL_INIT,
    _CTOR_SEQUENCE_CH_SYS_INIT,
    _CTOR_SEQUENCE_WORKER_THREADS_START,
    _CTOR_SEQUENCE_OMD_PUBSUB_TOPIC_INIT,
    _CTOR_SEQUENCE_OMD_PARAM_INIT,
    _CTOR_SEQUENCE_OMD_UAVCAN_INIT,
    _CTOR_SEQUENCE_LENGTH
};
