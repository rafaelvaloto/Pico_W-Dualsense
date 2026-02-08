//
// Created by rafaelvaloto on 01/02/2026.
//

#ifndef DUALSENSE_TEST_FLASH_OPERATOR_H
#define DUALSENSE_TEST_FLASH_OPERATOR_H

#include <cstdio>
#include <cstring>

#include "btstack_util.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

// offset 1.5MB
#define FLASH_TARGET_OFFSET (1536 * 1024)
#define CONFIG_VALID_MARKER 0xDEADBEEF
#define CONFIG_INVALID      0xFF

typedef struct {
    uint8_t mac[6];
    link_key_t link_key;
    uint32_t exists;
} device_config_t;

inline void flash_save_config(const uint8_t *addr, const uint8_t *link_key) {
    uint8_t buffer[FLASH_PAGE_SIZE] = {};
    device_config_t config;
    memcpy(config.mac, addr, 6);
    if (link_key) {
        memcpy(config.link_key, link_key, LINK_KEY_LEN);
        config.exists = CONFIG_VALID_MARKER;
    }

    memcpy(buffer, &config, sizeof(device_config_t));

    printf("[FLASH] Config saved: MAC=%s, Key: ", bd_addr_to_str(addr));
    if (link_key) {
        for (int i = 0; i < LINK_KEY_LEN; i++) {
            printf("%02X", link_key[i]);
        }
    }
    printf("\n");

    const uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, buffer, FLASH_PAGE_SIZE);
    restore_interrupts(ints);
}

inline bool flash_load_config(uint8_t *addr, uint8_t *link_key) {
    const auto flash_target_contents = reinterpret_cast<const device_config_t *>((XIP_BASE + FLASH_TARGET_OFFSET));

    if (flash_target_contents->exists == CONFIG_VALID_MARKER) {
        memcpy(addr, flash_target_contents->mac, 6);
        memcpy(link_key, flash_target_contents->link_key, LINK_KEY_LEN);
        printf("[FLASH] Config carregada: MAC=%s\n", bd_addr_to_str(addr));
        printf("[FLASH] Config saved: MAC=%s, Key: ", bd_addr_to_str(addr));
        if (link_key) {
            for (int i = 0; i < 16; i++) {
                printf("%02X", link_key[i]);
            }
        }
        printf("\n");
        return true;
    }

    return false;
}

inline void flash_clear_config() {
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);
    printf("[FLASH] clean keys!\n");
}

#endif //DUALSENSE_TEST_FLASH_OPERATOR_H
