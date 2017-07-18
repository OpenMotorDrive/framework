#include <common/flash.h>

bool __attribute__ ((noinline)) flash_program_half_word(uint16_t* addr, const uint16_t* src)
{
    bool ret;
    flash_unlock();
    // 1. Check that no main Flash memory operation is ongoing by checking the BSY bit in the FLASH_SR register.
    flash_wait_for_last_operation();
    // 2. Set the PG bit in the FLASH_CR register.
    FLASH_CR |= FLASH_CR_PG;
    // 3. Perform the data write (half-word) at the desired address.
    *addr = *src;
    // 4. Wait until the BSY bit is reset in the FLASH_SR register.
    flash_wait_for_last_operation();
    // 5. Check the EOP flag in the FLASH_SR register (it is set when the programming operation has succeded), and then clear it by software
    ret = (FLASH_SR & FLASH_SR_EOP) != 0 && (FLASH_SR & (1UL<<2)) == 0 && (FLASH_SR & (1UL<<4)) == 0;
    FLASH_SR &= ~FLASH_SR_EOP;
    // also clear PG for good measure
    FLASH_CR &= ~FLASH_CR_PG;

    flash_wait_for_last_operation();
    flash_lock();

    return ret;
}

bool __attribute__ ((noinline)) flash_erase_page(void* addr)
{
    flash_unlock();
    bool ret;
    // 1. Check that no Flash memory operation is ongoing by checking the BSY bit in the FLASH_CR register.
    flash_wait_for_last_operation();
    // 2. Set the PER bit in the FLASH_CR register
    FLASH_CR |= FLASH_CR_PER;
    // 3. Program the FLASH_AR register to select a page to erase
    FLASH_AR = (uint32_t)addr;
    // 4. Set the STRT bit in the FLASH_CR register (see below note)
    FLASH_CR |= FLASH_CR_STRT;
    // 5. Wait for the BSY bit to be reset
    flash_wait_for_last_operation();
    // 6. Check the EOP flag in the FLASH_SR register (it is set when the erase operation has succeded), and then clear it by software.
    ret = (FLASH_SR & FLASH_SR_EOP) != 0;
    FLASH_SR &= ~FLASH_SR_EOP;
    // also clear PER for good measure
    FLASH_CR &= ~FLASH_CR_PER;

    flash_lock();
    return ret;
}
