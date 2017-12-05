#include <ch.h>

#if !CH_DBG_SYSTEM_STATE_CHECK
#warning Consider enabling CH_DBG_SYSTEM_STATE_CHECK in framework_conf.h.
#endif

#if !CH_DBG_ENABLE_CHECKS
#warning Consider enabling CH_DBG_ENABLE_CHECKS in framework_conf.h.
#endif

#if !CH_DBG_ENABLE_ASSERTS
#warning Consider enabling CH_DBG_ENABLE_ASSERTS in framework_conf.h.
#endif

#if !CH_DBG_ENABLE_STACK_CHECK
#warning Consider enabling CH_DBG_ENABLE_STACK_CHECK in framework_conf.h.
#endif
