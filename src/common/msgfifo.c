#include <common/msgfifo.h>
#include <stdbool.h>

#define MSGFIFO_ALIGN(ptr) ((void*)(((size_t)(ptr) + (sizeof(void*)-1)) & ~(sizeof(void*)-1)))

static bool msgfifo_block_in_range(struct msgfifo_instance_s* instance, void* block, size_t block_size);
static void msgfifo_pop_oldest(struct msgfifo_instance_s* instance);

void msgfifo_init(struct msgfifo_instance_s* instance, size_t memory_pool_size, void* memory_pool) {
    if (!instance || !memory_pool) {
        return;
    }

    instance->memory_pool = memory_pool;
    instance->memory_pool_size = memory_pool_size;
    instance->newest = NULL;
    instance->oldest = NULL;
}

void* msgfifo_allocate(struct msgfifo_instance_s* instance, size_t msg_size, delete_handler_ptr delete_cb) {

    if (!instance || !instance->memory_pool) {
        return NULL;
    }

    size_t insert_block_size = msg_size+sizeof(struct msgfifo_block_s);

    while(true) {
        struct msgfifo_block_s* insert_block;
        if (instance->newest) {
            insert_block = (struct msgfifo_block_s*)((uint8_t*)instance->newest->msg + instance->newest->msg_size);
        } else {
            insert_block = (struct msgfifo_block_s*)instance->memory_pool;
        }

        insert_block = MSGFIFO_ALIGN(insert_block);

        // Check if the block to be inserted is inside the memory pool
        if (!msgfifo_block_in_range(instance, insert_block, insert_block_size)) {
            insert_block = (struct msgfifo_block_s*)instance->memory_pool;
            insert_block = MSGFIFO_ALIGN(insert_block);

            if (!msgfifo_block_in_range(instance, insert_block, insert_block_size)) {
                return NULL;
            }
        }

        if (instance->oldest && (size_t)instance->oldest < (size_t)insert_block+insert_block_size) {
            msgfifo_pop_oldest(instance);
            continue;
        }

        insert_block->next_oldest = NULL;
        insert_block->msg_size = msg_size;
        insert_block->delete_cb = delete_cb;

        instance->newest->next_oldest = insert_block;

        instance->newest = insert_block;

        return insert_block->msg;
    }
}

static void msgfifo_pop_oldest(struct msgfifo_instance_s* instance) {
    if (!instance || !instance->oldest) {
        return;
    }

    if (instance->oldest->delete_cb) {
        instance->oldest->delete_cb(instance->oldest->msg);
    }

    if (instance->newest == instance->oldest) {
        instance->newest = NULL;
    }

    instance->oldest = instance->oldest->next_oldest;
}

static bool msgfifo_block_in_range(struct msgfifo_instance_s* instance, void* block, size_t block_size) {
    return instance && instance->memory_pool &&
           (size_t)block-(size_t)instance->memory_pool < instance->memory_pool_size &&
           (size_t)block+block_size-(size_t)instance->memory_pool < instance->memory_pool_size;
}
