#include "lpwork_thread.h"

#include <common/ctor.h>

#ifndef LPWORK_THREAD_STACK_SIZE
#define LPWORK_THREAD_STACK_SIZE 1024
#endif

#ifndef LPWORK_THREAD_PRIORITY
#define LPWORK_THREAD_PRIORITY LOWPRIO
#endif

struct worker_thread_s lpwork_thread;

RUN_ON(WORKER_THREADS_START) {
    worker_thread_init(&lpwork_thread, LPWORK_THREAD_STACK_SIZE, LPWORK_THREAD_PRIORITY);
}
