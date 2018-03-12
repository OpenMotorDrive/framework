#include "flash.h"

#include <ch.h>
#include <hal.h>
#ifdef STM32F1xx_MCUCONF


#define FLASH_WORD_SIZE sizeof(flash_word_t)
typedef uint16_t flash_word_t;

uint32_t flash_getpageaddr(uint32_t page)
{
    return (page*2048);
}

static void __attribute__((noinline)) flash_wait_until_ready(void) {
    while(FLASH->SR & FLASH_SR_BSY);
}

static bool flash_unlock(void) {
    if (!(FLASH->CR & FLASH_CR_LOCK)) {
        return true;
    }

    FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xCDEF89AB;

    return !(FLASH->CR & FLASH_CR_LOCK);
}

static void flash_lock(void) {
    FLASH->CR |= FLASH_CR_LOCK;
}

static bool __attribute__((noinline)) flash_write_word(volatile flash_word_t* target, flash_word_t data) {
    if (((size_t)target % FLASH_WORD_SIZE) != 0) {
        return false;
    }

    // Clear STRT bit
    FLASH->CR &= ~FLASH_CR_STRT;
    // 1. Check that no main Flash memory operation is ongoing by checking the
    //    BSY bit in the FLASH_SR register.
    flash_wait_until_ready();
    // 2. Set the PG bit in the FLASH_CR register.
    FLASH->CR |= FLASH_CR_PG;
    // 3. Perform the data write (half-word) at the desired address.
    *target = data;
    // 4. Wait until the BSY bit is reset in the FLASH_SR register.
    flash_wait_until_ready();
    // 5. Check the EOP flag in the FLASH_SR register (it is set when the
    //    programming operation has succeeded), and then clear it by software.
    bool success = FLASH->SR & FLASH_SR_EOP;

    FLASH->SR |= FLASH_SR_EOP;

    return success;
}

bool flash_write(void* address, volatile uint8_t num_bufs, struct flash_write_buf_s* volatile bufs) {
    if (num_bufs == 0 || !address || (size_t)address % FLASH_WORD_SIZE != 0 || !flash_unlock()) {
        return false;
    }

    bool success = true;
    flash_word_t* target_word_ptr = address;
    uint8_t buf_idx = 0;
    size_t buf_data_idx = 0;

    while (buf_data_idx >= bufs[buf_idx].len) {
        buf_idx++;
    }

    while (buf_idx < num_bufs) {
        union {
            flash_word_t word_value;
            uint8_t bytes_value[FLASH_WORD_SIZE];
        } source_word;

        memset(&source_word,0,sizeof(source_word));

        for (uint8_t i=0; i<FLASH_WORD_SIZE; i++) {
            if (buf_idx >= num_bufs) {
                break;
            }
            source_word.bytes_value[i] = ((uint8_t*)bufs[buf_idx].data)[buf_data_idx];
            buf_data_idx++;
            while (buf_data_idx >= bufs[buf_idx].len) {
                buf_idx++;
                buf_data_idx = 0;
            }
        }

        flash_write_word(target_word_ptr, source_word.word_value);

        target_word_ptr++;
    }

    flash_lock();

    return success;
}

bool __attribute__((noinline)) flash_erase_page(void* page_addr) {
    if (!flash_unlock()) {
        return false;
    }

    // 1. Check that no Flash memory operation is ongoing by checking the BSY
    //    bit in the FLASH_CR register
    flash_wait_until_ready();

    // 2. Set the PER bit in the FLASH_CR register
    FLASH->CR |= FLASH_CR_PER;

    // 3. Program the FLASH_AR register to select a page to erase
    FLASH->AR = (uint32_t)page_addr;

    // 4. Set the STRT bit in the FLASH_CR register
    FLASH->CR |= FLASH_CR_STRT;

    // NOTE: The software should start checking if the BSY bit equals ‘0’ at
    //       least one CPU cycle after setting the STRT bit.
    __asm__("nop");

    // 5. Wait for the BSY bit to be reset
    flash_wait_until_ready();

    // 6. Check the EOP flag in the FLASH_SR register (it is set when the erase
    //    operation has succeeded), and then clear it by software.
    bool success = FLASH->SR & FLASH_SR_EOP;
    FLASH->SR |= FLASH_SR_EOP;
    // unset the PER bit
    FLASH->CR &= ~FLASH_CR_PER;
    flash_lock();
    return success;
}
#endif //STM32F1xx_MCUCONF
