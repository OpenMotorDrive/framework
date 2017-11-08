#include "fifoallocator.h"
#include <stdbool.h>

#define FIFOALLOCATOR_ALIGN(ptr) ((void*)(((size_t)(ptr) + (sizeof(void*)-1)) & ~(sizeof(void*)-1)))

static bool fifoallocator_block_in_range(struct fifoallocator_instance_s* instance, void* block, size_t block_size);
static void fifoallocator_pop_oldest(struct fifoallocator_instance_s* instance);

void fifoallocator_init(struct fifoallocator_instance_s* instance, size_t memory_pool_size, void* memory_pool, delete_handler_ptr delete_cb) {
    if (!instance || !memory_pool) {
        return;
    }

    instance->memory_pool = memory_pool;
    instance->memory_pool_size = memory_pool_size;
    instance->newest = NULL;
    instance->oldest = NULL;
    instance->delete_cb = delete_cb;
}

void* fifoallocator_allocate(struct fifoallocator_instance_s* instance, size_t data_size) {

    if (!instance || !instance->memory_pool) {
        return NULL;
    }

    size_t insert_block_size = data_size+sizeof(struct fifoallocator_block_s);

    struct fifoallocator_block_s* insert_block;
    if (instance->newest) {
        insert_block = (struct fifoallocator_block_s*)((uint8_t*)instance->newest->data + instance->newest->data_size);
    } else {
        insert_block = (struct fifoallocator_block_s*)instance->memory_pool;
    }

    insert_block = FIFOALLOCATOR_ALIGN(insert_block);

    // Check if the block to be inserted is inside the memory pool
    if (!fifoallocator_block_in_range(instance, insert_block, insert_block_size)) {
        // The block doesn't fit. Move it to the beginning of the memory pool.
        insert_block = FIFOALLOCATOR_ALIGN((struct fifoallocator_block_s*)instance->memory_pool);

        if (!fifoallocator_block_in_range(instance, insert_block, insert_block_size)) {
            return NULL;
        }

        // Delete oldest block until there is no wrapping.
        while ((size_t)instance->oldest > (size_t)instance->newest) {
            fifoallocator_pop_oldest(instance);
        }
    }

    // Check if the insert block overlaps with the oldest block
    while (instance->oldest && (size_t)instance->oldest >= (size_t)insert_block && (size_t)instance->oldest < (size_t)insert_block+insert_block_size) {
        fifoallocator_pop_oldest(instance);
    }

    // Check if we've deleted everything, start from the beginning if so
    if (!instance->newest) {
        insert_block = FIFOALLOCATOR_ALIGN((struct fifoallocator_block_s*)instance->memory_pool);
    }

    insert_block->next_oldest = NULL;
    insert_block->data_size = data_size;

    if (instance->newest) {
        instance->newest->next_oldest = insert_block;
    }

    instance->newest = insert_block;

    if (!instance->oldest) {
        instance->oldest = insert_block;
    }

    return insert_block->data;
}

size_t fifoallocator_get_block_size(const void* block) {
    if (!block) {
        return 0;
    }

    return ((struct fifoallocator_block_s*)((uint8_t*)block - offsetof(struct fifoallocator_block_s, data)))->data_size;
}

static void fifoallocator_pop_oldest(struct fifoallocator_instance_s* instance) {
    if (!instance || !instance->oldest) {
        return;
    }

    if (instance->delete_cb) {
        instance->delete_cb(instance->oldest->data);
    }

    if (instance->newest == instance->oldest) {
        instance->newest = NULL;
    }

    instance->oldest = instance->oldest->next_oldest;
}

static bool fifoallocator_block_in_range(struct fifoallocator_instance_s* instance, void* block, size_t block_size) {
    return instance && instance->memory_pool &&
           (size_t)block-(size_t)instance->memory_pool < instance->memory_pool_size &&
           (size_t)block+block_size-(size_t)instance->memory_pool <= instance->memory_pool_size;
}
