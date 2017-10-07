#pragma once

#include <stddef.h>
#include <stdint.h>

typedef void (*delete_handler_ptr)(void* block);

struct msgfifo_block_s {
    struct msgfifo_block_s* next_oldest;
    delete_handler_ptr delete_cb;
    size_t msg_size;
    uint8_t msg[];
};

struct msgfifo_instance_s {
    void* memory_pool;
    size_t memory_pool_size;
    struct msgfifo_block_s* newest;
    struct msgfifo_block_s* oldest;
};

void msgfifo_init(struct msgfifo_instance_s* instance, size_t memory_pool_size, void* memory_pool);
void* msgfifo_allocate(struct msgfifo_instance_s* instance, size_t msg_size, delete_handler_ptr delete_cb);
