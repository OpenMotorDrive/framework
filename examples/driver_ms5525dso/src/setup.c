#include <modules/pubsub/pubsub.h>
#include <modules/worker_thread/worker_thread.h>

WORKER_THREAD_SPAWN(lpwork_thread, LOWPRIO, 1024)
WORKER_THREAD_SPAWN(can_thread, LOWPRIO, 1024)
WORKER_THREAD_TAKEOVER_MAIN(hpwork_thread, HIGHPRIO)

PUBSUB_TOPIC_GROUP_CREATE(default_topic_group, 1024)
