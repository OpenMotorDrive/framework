#include <modules/worker_thread/worker_thread.h>
#include <modules/pubsub/pubsub.h>

PUBSUB_TOPIC_GROUP_CREATE(default_topic_group, 2048)

WORKER_THREAD_SPAWN(can_thread, LOWPRIO, 1024)
WORKER_THREAD_TAKEOVER_MAIN(lpwork_thread, HIGHPRIO)
