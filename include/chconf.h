#pragma once

#include <framework_conf.h>

#define _CHIBIOS_RT_CONF_

/**
 * @brief   OS optimization.
 * @details If enabled then time efficient rather than space efficient code
 *          is used when two possible implementations exist.
 *
 * @note    This is not related to the compiler optimization options.
 * @note    The default is @p TRUE.
 */
#ifndef CH_CFG_OPTIMIZE_SPEED
#define CH_CFG_OPTIMIZE_SPEED               FALSE
#endif

/**
 * @brief   System time counter resolution.
 * @note    Allowed values are 16 or 32 bits.
 */
#ifndef CH_CFG_ST_RESOLUTION
#define CH_CFG_ST_RESOLUTION                16
#endif

/**
 * @brief   System tick frequency.
 * @details Frequency of the system timer that drives the system ticks. This
 *          setting also defines the system tick time unit.
 */
#ifndef CH_CFG_ST_FREQUENCY
#define CH_CFG_ST_FREQUENCY                 1000000
#endif

/**
 * @brief   Time delta constant for the tick-less mode.
 * @note    If this value is zero then the system uses the classic
 *          periodic tick. This value represents the minimum number
 *          of ticks that is safe to specify in a timeout directive.
 *          The value one is not valid, timeouts are rounded up to
 *          this value.
 */
#ifndef CH_CFG_ST_TIMEDELTA
#define CH_CFG_ST_TIMEDELTA                 2
#endif

/**
 * @brief   Round robin interval.
 * @details This constant is the number of system ticks allowed for the
 *          threads before preemption occurs. Setting this value to zero
 *          disables the preemption for threads with equal priority and the
 *          round robin becomes cooperative. Note that higher priority
 *          threads can still preempt, the kernel is always preemptive.
 * @note    Disabling the round robin preemption makes the kernel more compact
 *          and generally faster.
 * @note    The round robin preemption is not supported in tickless mode and
 *          must be set to zero in that case.
 */
#ifndef CH_CFG_TIME_QUANTUM
#define CH_CFG_TIME_QUANTUM                 0
#endif

/**
 * @brief   Idle thread automatic spawn suppression.
 * @details When this option is activated the function @p chSysInit()
 *          does not spawn the idle thread. The application @p main()
 *          function becomes the idle thread and must implement an
 *          infinite loop.
 */
#ifndef CH_CFG_NO_IDLE_THREAD
#define CH_CFG_NO_IDLE_THREAD               FALSE
#endif

/**
 * @brief   Time Measurement APIs.
 * @details If enabled then the time measurement APIs are included in
 *          the kernel.
 *
 * @note    The default is @p TRUE.
 */
#ifndef CH_CFG_USE_TM
#define CH_CFG_USE_TM                       FALSE
#endif

/**
 * @brief   Threads registry APIs.
 * @details If enabled then the registry APIs are included in the kernel.
 *
 * @note    The default is @p TRUE.
 */
#ifndef CH_CFG_USE_REGISTRY
#define CH_CFG_USE_REGISTRY                 TRUE
#endif

/**
 * @brief   Threads synchronization APIs.
 * @details If enabled then the @p chThdWait() function is included in
 *          the kernel.
 *
 * @note    The default is @p TRUE.
 */
#ifndef CH_CFG_USE_WAITEXIT
#define CH_CFG_USE_WAITEXIT                 FALSE
#endif

/**
 * @brief   Semaphores APIs.
 * @details If enabled then the Semaphores APIs are included in the kernel.
 *
 * @note    The default is @p TRUE.
 */
#ifndef CH_CFG_USE_SEMAPHORES
#define CH_CFG_USE_SEMAPHORES               FALSE
#endif

/**
 * @brief   Semaphores queuing mode.
 * @details If enabled then the threads are enqueued on semaphores by
 *          priority rather than in FIFO order.
 *
 * @note    The default is @p FALSE. Enable this if you have special
 *          requirements.
 * @note    Requires @p CH_CFG_USE_SEMAPHORES.
 */
#ifndef CH_CFG_USE_SEMAPHORES_PRIORITY
#define CH_CFG_USE_SEMAPHORES_PRIORITY      FALSE
#endif

/**
 * @brief   Mutexes APIs.
 * @details If enabled then the mutexes APIs are included in the kernel.
 *
 * @note    The default is @p TRUE.
 */
#ifndef CH_CFG_USE_MUTEXES
#define CH_CFG_USE_MUTEXES                  TRUE
#endif

/**
 * @brief   Enables recursive behavior on mutexes.
 * @note    Recursive mutexes are heavier and have an increased
 *          memory footprint.
 *
 * @note    The default is @p FALSE.
 * @note    Requires @p CH_CFG_USE_MUTEXES.
 */
#ifndef CH_CFG_USE_MUTEXES_RECURSIVE
#define CH_CFG_USE_MUTEXES_RECURSIVE        TRUE
#endif

/**
 * @brief   Conditional Variables APIs.
 * @details If enabled then the conditional variables APIs are included
 *          in the kernel.
 *
 * @note    The default is @p TRUE.
 * @note    Requires @p CH_CFG_USE_MUTEXES.
 */
#ifndef CH_CFG_USE_CONDVARS
#define CH_CFG_USE_CONDVARS                 FALSE
#endif

/**
 * @brief   Conditional Variables APIs with timeout.
 * @details If enabled then the conditional variables APIs with timeout
 *          specification are included in the kernel.
 *
 * @note    The default is @p TRUE.
 * @note    Requires @p CH_CFG_USE_CONDVARS.
 */
#ifndef CH_CFG_USE_CONDVARS_TIMEOUT
#define CH_CFG_USE_CONDVARS_TIMEOUT         FALSE
#endif

/**
 * @brief   Events Flags APIs.
 * @details If enabled then the event flags APIs are included in the kernel.
 *
 * @note    The default is @p TRUE.
 */
#ifndef CH_CFG_USE_EVENTS
#define CH_CFG_USE_EVENTS                   FALSE
#endif

/**
 * @brief   Events Flags APIs with timeout.
 * @details If enabled then the events APIs with timeout specification
 *          are included in the kernel.
 *
 * @note    The default is @p TRUE.
 * @note    Requires @p CH_CFG_USE_EVENTS.
 */
#ifndef CH_CFG_USE_EVENTS_TIMEOUT
#define CH_CFG_USE_EVENTS_TIMEOUT           FALSE
#endif

/**
 * @brief   Synchronous Messages APIs.
 * @details If enabled then the synchronous messages APIs are included
 *          in the kernel.
 *
 * @note    The default is @p TRUE.
 */
#ifndef CH_CFG_USE_MESSAGES
#define CH_CFG_USE_MESSAGES                 FALSE
#endif

/**
 * @brief   Synchronous Messages queuing mode.
 * @details If enabled then messages are served by priority rather than in
 *          FIFO order.
 *
 * @note    The default is @p FALSE. Enable this if you have special
 *          requirements.
 * @note    Requires @p CH_CFG_USE_MESSAGES.
 */
#ifndef CH_CFG_USE_MESSAGES_PRIORITY
#define CH_CFG_USE_MESSAGES_PRIORITY        FALSE
#endif

/**
 * @brief   Mailboxes APIs.
 * @details If enabled then the asynchronous messages (mailboxes) APIs are
 *          included in the kernel.
 *
 * @note    The default is @p TRUE.
 * @note    Requires @p CH_CFG_USE_SEMAPHORES.
 */
#ifndef CH_CFG_USE_MAILBOXES
#define CH_CFG_USE_MAILBOXES                TRUE
#endif

/**
 * @brief   Core Memory Manager APIs.
 * @details If enabled then the core memory manager APIs are included
 *          in the kernel.
 *
 * @note    The default is @p TRUE.
 */
#ifndef CH_CFG_USE_MEMCORE
#define CH_CFG_USE_MEMCORE                  TRUE
#endif

/**
 * @brief   Heap Allocator APIs.
 * @details If enabled then the memory heap allocator APIs are included
 *          in the kernel.
 *
 * @note    The default is @p TRUE.
 * @note    Requires @p CH_CFG_USE_MEMCORE and either @p CH_CFG_USE_MUTEXES or
 *          @p CH_CFG_USE_SEMAPHORES.
 * @note    Mutexes are recommended.
 */
#ifndef CH_CFG_USE_HEAP
#define CH_CFG_USE_HEAP                     FALSE
#endif

/**
 * @brief   Memory Pools Allocator APIs.
 * @details If enabled then the memory pools allocator APIs are included
 *          in the kernel.
 *
 * @note    The default is @p TRUE.
 */
#ifndef CH_CFG_USE_MEMPOOLS
#define CH_CFG_USE_MEMPOOLS                 TRUE
#endif

/**
 * @brief   Managed RAM size.
 * @details Size of the RAM area to be managed by the OS. If set to zero
 *          then the whole available RAM is used. The core memory is made
 *          available to the heap allocator and/or can be used directly through
 *          the simplified core memory allocator.
 *
 * @note    In order to let the OS manage the whole RAM the linker script must
 *          provide the @p __heap_base__ and @p __heap_end__ symbols.
 * @note    Requires @p CH_CFG_USE_MEMCORE.
 */
#ifndef CH_CFG_MEMCORE_SIZE
#define CH_CFG_MEMCORE_SIZE                 0
#endif


/**
 * @brief   Dynamic Threads APIs.
 * @details If enabled then the dynamic threads creation APIs are included
 *          in the kernel.
 *
 * @note    The default is @p TRUE.
 * @note    Requires @p CH_CFG_USE_WAITEXIT.
 * @note    Requires @p CH_CFG_USE_HEAP and/or @p CH_CFG_USE_MEMPOOLS.
 */
#ifndef CH_CFG_USE_DYNAMIC
#define CH_CFG_USE_DYNAMIC                  FALSE
#endif

/**
 * @brief   Debug option, kernel statistics.
 *
 * @note    The default is @p FALSE.
 */
#ifndef CH_DBG_STATISTICS
#define CH_DBG_STATISTICS                   FALSE
#endif

/**
 * @brief   Debug option, system state check.
 * @details If enabled the correct call protocol for system APIs is checked
 *          at runtime.
 *
 * @note    The default is @p FALSE.
 */
#ifndef CH_DBG_SYSTEM_STATE_CHECK
#define CH_DBG_SYSTEM_STATE_CHECK           FALSE
#endif

/**
 * @brief   Debug option, parameters checks.
 * @details If enabled then the checks on the API functions input
 *          parameters are activated.
 *
 * @note    The default is @p FALSE.
 */
#ifndef CH_DBG_ENABLE_CHECKS
#define CH_DBG_ENABLE_CHECKS                FALSE
#endif

/**
 * @brief   Debug option, consistency checks.
 * @details If enabled then all the assertions in the kernel code are
 *          activated. This includes consistency checks inside the kernel,
 *          runtime anomalies and port-defined checks.
 *
 * @note    The default is @p FALSE.
 */
#ifndef CH_DBG_ENABLE_ASSERTS
#define CH_DBG_ENABLE_ASSERTS               FALSE
#endif

/**
 * @brief   Debug option, trace buffer.
 * @details If enabled then the trace buffer is activated.
 *
 * @note    The default is @p CH_DBG_TRACE_MASK_DISABLED.
 */
#ifndef CH_DBG_TRACE_MASK
#define CH_DBG_TRACE_MASK                   CH_DBG_TRACE_MASK_DISABLED
#endif

/**
 * @brief   Trace buffer entries.
 * @note    The trace buffer is only allocated if @p CH_DBG_TRACE_MASK is
 *          different from @p CH_DBG_TRACE_MASK_DISABLED.
 */
#ifndef CH_DBG_TRACE_BUFFER_SIZE
#define CH_DBG_TRACE_BUFFER_SIZE            128
#endif

/**
 * @brief   Debug option, stack checks.
 * @details If enabled then a runtime stack check is performed.
 *
 * @note    The default is @p FALSE.
 * @note    The stack check is performed in a architecture/port dependent way.
 *          It may not be implemented or some ports.
 * @note    The default failure mode is to halt the system with the global
 *          @p panic_msg variable set to @p NULL.
 */
#ifndef CH_DBG_ENABLE_STACK_CHECK
#define CH_DBG_ENABLE_STACK_CHECK           FALSE
#endif

/**
 * @brief   Debug option, stacks initialization.
 * @details If enabled then the threads working area is filled with a byte
 *          value when a thread is created. This can be useful for the
 *          runtime measurement of the used stack.
 *
 * @note    The default is @p FALSE.
 */
#ifndef CH_DBG_FILL_THREADS
#define CH_DBG_FILL_THREADS                 FALSE
#endif

/**
 * @brief   Debug option, threads profiling.
 * @details If enabled then a field is added to the @p thread_t structure that
 *          counts the system ticks occurred while executing the thread.
 *
 * @note    The default is @p FALSE.
 * @note    This debug option is not currently compatible with the
 *          tickless mode.
 */
#ifndef CH_DBG_THREADS_PROFILING
#define CH_DBG_THREADS_PROFILING            FALSE
#endif

/**
 * @brief   Threads descriptor structure extension.
 * @details User fields added to the end of the @p thread_t structure.
 */
#ifndef CH_CFG_THREAD_EXTRA_FIELDS
#define CH_CFG_THREAD_EXTRA_FIELDS                                          \
/* Add threads custom fields here.*/
#endif

/**
 * @brief   Threads initialization hook.
 * @details User initialization code added to the @p chThdInit() API.
 *
 * @note    It is invoked from within @p chThdInit() and implicitly from all
 *          the threads creation APIs.
 */
#ifndef CH_CFG_THREAD_INIT_HOOK
#define CH_CFG_THREAD_INIT_HOOK(tp) {                                       \
/* Add threads initialization code here.*/                                \
}
#endif

/**
 * @brief   Threads finalization hook.
 * @details User finalization code added to the @p chThdExit() API.
 */
#ifndef CH_CFG_THREAD_EXIT_HOOK
#define CH_CFG_THREAD_EXIT_HOOK(tp) {                                       \
/* Add threads finalization code here.*/                                  \
}
#endif

/**
 * @brief   Context switch hook.
 * @details This hook is invoked just before switching between threads.
 */
#ifndef CH_CFG_CONTEXT_SWITCH_HOOK
#define CH_CFG_CONTEXT_SWITCH_HOOK(ntp, otp) {                              \
/* Context switch code here.*/                                            \
}
#endif

/**
 * @brief   ISR enter hook.
 */
#ifndef CH_CFG_IRQ_PROLOGUE_HOOK
#define CH_CFG_IRQ_PROLOGUE_HOOK() {                                        \
/* IRQ prologue code here.*/                                              \
}
#endif

/**
 * @brief   ISR exit hook.
 */
#ifndef CH_CFG_IRQ_EPILOGUE_HOOK
#define CH_CFG_IRQ_EPILOGUE_HOOK() {                                        \
/* IRQ epilogue code here.*/                                              \
}
#endif

/**
 * @brief   Idle thread enter hook.
 * @note    This hook is invoked within a critical zone, no OS functions
 *          should be invoked from here.
 * @note    This macro can be used to activate a power saving mode.
 */
#ifndef CH_CFG_IDLE_ENTER_HOOK
#define CH_CFG_IDLE_ENTER_HOOK() {                                          \
/* Idle-enter code here.*/                                                \
}
#endif

/**
 * @brief   Idle thread leave hook.
 * @note    This hook is invoked within a critical zone, no OS functions
 *          should be invoked from here.
 * @note    This macro can be used to deactivate a power saving mode.
 */
#ifndef CH_CFG_IDLE_LEAVE_HOOK
#define CH_CFG_IDLE_LEAVE_HOOK() {                                          \
/* Idle-leave code here.*/                                                \
}
#endif

/**
 * @brief   Idle Loop hook.
 * @details This hook is continuously invoked by the idle thread loop.
 */
#ifndef CH_CFG_IDLE_LOOP_HOOK
#define CH_CFG_IDLE_LOOP_HOOK() {                                           \
/* Idle loop code here.*/                                                 \
}
#endif

/**
 * @brief   System tick event hook.
 * @details This hook is invoked in the system tick handler immediately
 *          after processing the virtual timers queue.
 */
#ifndef CH_CFG_SYSTEM_TICK_HOOK
#define CH_CFG_SYSTEM_TICK_HOOK() {                                         \
/* System tick event code here.*/                                         \
}
#endif

/**
 * @brief   System halt hook.
 * @details This hook is invoked in case to a system halting error before
 *          the system is halted.
 */
#ifndef CH_CFG_SYSTEM_HALT_HOOK
#define CH_CFG_SYSTEM_HALT_HOOK(reason) {                                   \
/* System halt code here.*/                                               \
}
#endif

/**
 * @brief   Trace hook.
 * @details This hook is invoked each time a new record is written in the
 *          trace buffer.
 */
#ifndef CH_CFG_TRACE_HOOK
#define CH_CFG_TRACE_HOOK(tep) {                                            \
/* Trace code here.*/                                                     \
}
#endif

#ifndef CH_CFG_CORE_ALLOCATOR_FAILURE_HOOK
#define CH_CFG_CORE_ALLOCATOR_FAILURE_HOOK() {}
#endif
