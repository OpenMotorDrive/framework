#pragma once

#include <stddef.h>
#include <stdint.h>
#include <ch.h>

typedef void (*delete_handler_ptr)(void* block);

struct fifoallocator_block_s {
    struct fifoallocator_block_s* next_oldest;
    size_t data_size;
    uint8_t data[];
};

struct fifoallocator_instance_s {
    void* memory_pool;
    size_t memory_pool_size;
    delete_handler_ptr delete_cb;
    struct fifoallocator_block_s* newest;
    struct fifoallocator_block_s* oldest;
    mutex_t mtx;
};

void fifoallocator_init(struct fifoallocator_instance_s* instance, size_t memory_pool_size, void* memory_pool, delete_handler_ptr delete_cb);
void* fifoallocator_allocate(struct fifoallocator_instance_s* instance, size_t msg_size);
void fifoallocator_lock(struct fifoallocator_instance_s* instance);
void fifoallocator_unlock(struct fifoallocator_instance_s* instance);
