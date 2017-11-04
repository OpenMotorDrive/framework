#pragma once

#include <ch.h>

struct __attribute__((packed)) flash_journal_entry_s {
    uint16_t crc16; // computed by xor-folding crc32
    uint8_t len; // valid range 0..254 - 0xff means entry is invalid
    uint8_t data[];
};

struct flash_journal_instance_s {
    struct flash_journal_entry_s* flash_page_ptr;
    size_t flash_page_size;
};

void flash_journal_init(struct flash_journal_instance_s* instance, void* flash_page_ptr, size_t flash_page_size);
bool flash_journal_iterate(struct flash_journal_instance_s* instance, const struct flash_journal_entry_s** entry_ptr);
bool flash_journal_write_from_2_buffers(struct flash_journal_instance_s* instance, size_t entry_buf1_size, const void* entry_buf1, size_t entry_buf2_size, const void* entry_buf2);
bool flash_journal_write(struct flash_journal_instance_s* instance, size_t entry_buf1_size, const void* entry_buf1);
bool flash_journal_erase(struct flash_journal_instance_s* instance);
uint32_t flash_journal_count_entries(struct flash_journal_instance_s* instance);
