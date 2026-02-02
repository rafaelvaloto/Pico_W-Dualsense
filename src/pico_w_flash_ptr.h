//
// Created by rafaelvaloto on 01/02/2026.
//

#ifndef DUALSENSE_TEST_FLASH_OPERATOR_H
#define DUALSENSE_TEST_FLASH_OPERATOR_H

#include <cstdio>
#include <cstring>

#include "hardware/flash.h"
#include "hardware/sync.h"

// offset 1.5MB
#define FLASH_TARGET_OFFSET (1536 * 1024)

inline void flash_alloc_macaddress(const uint8_t *addr) {
    uint8_t buffer[FLASH_PAGE_SIZE] = {};
    memcpy(buffer, addr, 6); // BD_ADDR 6 bytes

    const uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, buffer, FLASH_PAGE_SIZE);

    restore_interrupts(ints);
    printf("Macaddress Saved On Flash!\n");
}

inline void flash_read_macaddress(uint8_t *addr) {
    const auto flash_target_contents = reinterpret_cast<const uint8_t *>((XIP_BASE + FLASH_TARGET_OFFSET));
    memcpy(addr, flash_target_contents, 6);
}

#endif //DUALSENSE_TEST_FLASH_OPERATOR_H
