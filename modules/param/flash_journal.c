#include "flash_journal.h"
#include <flash/flash.h>
#include <common/helpers.h>

#define FLASH_JOURNAL_ALIGNMENT 2
#define FLASH_JOURNAL_ENTRY_HEADER_SIZE sizeof(struct flash_journal_entry_s)

static uint16_t flash_journal_entry_compute_crc16(const struct flash_journal_entry_s* entry);
static bool flash_journal_entry_valid(struct flash_journal_instance_s* instance, const struct flash_journal_entry_s* entry);
static bool flash_journal_address_in_bounds(struct flash_journal_instance_s* instance, const void* address);
static bool flash_journal_range_in_bounds(struct flash_journal_instance_s* instance, const void* address, uint32_t len);
static size_t flash_journal_entry_size(uint8_t len);

void flash_journal_init(struct flash_journal_instance_s* instance, void* flash_page_ptr, size_t flash_page_size) {
    if (!instance) {
        return;
    }

    instance->flash_page_ptr = flash_page_ptr;
    instance->flash_page_size = flash_page_size;
}

bool flash_journal_write_from_2_buffers(struct flash_journal_instance_s* instance, size_t entry_buf1_size, const void* entry_buf1, size_t entry_buf2_size, const void* entry_buf2) {
    size_t total_size = entry_buf1_size+entry_buf2_size;
    if (!instance || total_size >= 0xFF) {
        return false;
    }

    struct flash_journal_entry_s* new_entry_ptr = NULL;
    while (flash_journal_iterate(instance, (const struct flash_journal_entry_s**)&new_entry_ptr));

    if (!flash_journal_range_in_bounds(instance, new_entry_ptr, flash_journal_entry_size(total_size))) {
        return false;
    }

    struct flash_journal_entry_s entry_header = {0};
    entry_header.len = total_size;

    // compute crc16
    entry_header.crc16 = crc16_ccitt((uint8_t*)&entry_header+sizeof(entry_header.crc16), FLASH_JOURNAL_ENTRY_HEADER_SIZE-sizeof(entry_header.crc16), entry_header.crc16);

    if (entry_buf1) {
        entry_header.crc16 = crc16_ccitt(entry_buf1, entry_buf1_size, entry_header.crc16);
    }

    if (entry_buf2) {
        entry_header.crc16 = crc16_ccitt(entry_buf2, entry_buf2_size, entry_header.crc16);
    }

    struct flash_write_buf_s write_bufs[] = {
        {sizeof(entry_header), &entry_header},
        {entry_buf1_size, entry_buf1},
        {entry_buf2_size, entry_buf2}
    };

    return flash_write(new_entry_ptr, 3, write_bufs);
}

bool flash_journal_write(struct flash_journal_instance_s* instance, size_t entry_buf1_size, const void* entry_buf1) {
    return flash_journal_write_from_2_buffers(instance, entry_buf1_size, entry_buf1, 0, NULL);
}

uint32_t flash_journal_count_entries(struct flash_journal_instance_s* instance) {
    if (!instance) {
        return 0;
    }

    uint32_t ret = 0;
    const struct flash_journal_entry_s* entry = NULL;
    while(flash_journal_iterate(instance, &entry)) {
        ret++;
    }

    return ret;
}

bool flash_journal_erase(struct flash_journal_instance_s* instance) {
    if (!instance) {
        return false;
    }

    return flash_erase_page(instance->flash_page_ptr);
}

bool flash_journal_iterate(struct flash_journal_instance_s* instance, const struct flash_journal_entry_s** entry_ptr) {
    if (!instance || !entry_ptr) {
        return false;
    }

    if (!*entry_ptr) {
        *entry_ptr = instance->flash_page_ptr;
    } else if(flash_journal_entry_valid(instance, *entry_ptr)) {
        size_t entry_size = flash_journal_entry_size((*entry_ptr)->len);
        *entry_ptr = (const struct flash_journal_entry_s*)((uint8_t*)*entry_ptr + entry_size);
    }

    return flash_journal_entry_valid(instance, *entry_ptr);
}

static bool flash_journal_entry_valid(struct flash_journal_instance_s* instance, const struct flash_journal_entry_s* entry) {
    if (!flash_journal_address_in_bounds(instance, entry) || entry->len == 0xff) {
        return false;
    }

    volatile size_t entry_size = flash_journal_entry_size(entry->len);
    volatile uint16_t crc16_computed = flash_journal_entry_compute_crc16(entry);
    volatile bool range_in_bounds = flash_journal_range_in_bounds(instance, entry, entry_size);

    return range_in_bounds && crc16_computed == entry->crc16;
}

static bool flash_journal_range_in_bounds(struct flash_journal_instance_s* instance, const void* address, uint32_t len) {
    return flash_journal_address_in_bounds(instance, address) && flash_journal_address_in_bounds(instance, (uint8_t*)address+len);
}

static bool flash_journal_address_in_bounds(struct flash_journal_instance_s* instance, const void* address) {
    if (!instance) {
        return false;
    }

    return address >= (void*)instance->flash_page_ptr && address < (void*)((uint8_t*)instance->flash_page_ptr+instance->flash_page_size);
}

static size_t flash_journal_entry_size(uint8_t len) {
    size_t ret = FLASH_JOURNAL_ENTRY_HEADER_SIZE + len + FLASH_JOURNAL_ALIGNMENT - 1;
    ret -= ret % FLASH_JOURNAL_ALIGNMENT;
    return ret;
}

static uint16_t flash_journal_entry_compute_crc16(const struct flash_journal_entry_s* entry) {
    if (!entry) {
        return 0;
    }

    uint16_t crc16 = 0;
    crc16 = crc16_ccitt((uint8_t*)entry+sizeof(entry->crc16), FLASH_JOURNAL_ENTRY_HEADER_SIZE-sizeof(entry->crc16), crc16);
    crc16 = crc16_ccitt(entry->data, entry->len, crc16);
    return crc16;
}
