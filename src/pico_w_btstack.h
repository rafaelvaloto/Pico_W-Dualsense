
//
// Created by rafaelvaloto on 04/02/2026.
//
#pragma once
#include "GCore/Interfaces/ISonyGamepad.h"

#include <cstdint>
#include <cstdio>
#include "bluetooth.h"
#include "btstack_event.h"
#include "l2cap.h"
#include "pico_w_flash_ptr.h"
#include "classic/hid_host.h"
#include "classic/sdp_server.h"

// Connection state
static uint16_t response_report = 0;
static uint16_t l2cap_cid_control = 0;
static uint16_t l2cap_cid_interrupt = 0;
static uint16_t l2cap_cid_out_interrupt = 0;
static bd_addr_t current_device_addr;
static bool device_found = false;
static bool is_pairing = false;
static bool we_initiated_connection = false;
static bool link_key_used = false;       // Flag: tried to use a saved link key
static uint8_t auth_failure_count = 0;   // Consecutive failures counter
static btstack_packet_callback_registration_t hci_event_callback;
static btstack_packet_callback_registration_t l2cap_event_callback;

// Helper: check if link key is valid (non-zero)
inline bool is_link_key_valid(const uint8_t *key) {
    for (int i = 0; i < 16; i++) {
        if (key[i] != 0) return true;
    }
    return false;
}

inline void reset_connection_state() {
    response_report = 0;
    l2cap_cid_control = 0;
    l2cap_cid_interrupt = 0;
    l2cap_cid_out_interrupt = 0;
    is_pairing = false;
    we_initiated_connection = false;
    link_key_used = false;
    memset(current_device_addr, 0, sizeof(current_device_addr));
}

inline void enable_discovery() {
    printf("[BT] Enabling discovery...\n");
    gap_connectable_control(1);
    gap_discoverable_control(1);

    // Check if device is already paired
    bd_addr_t saved_mac;
    link_key_t saved_key;
    if (flash_load_config(saved_mac, saved_key) && is_link_key_valid(saved_key)) {
        // Already paired - just wait for controller connection
        printf("[BT] Waiting reconnection from %s...\n", bd_addr_to_str(saved_mac));
        bd_addr_copy(current_device_addr, saved_mac);
    } else {
        // Not paired - start active search
        printf("[BT] Starting search for new devices...\n");
        is_pairing = true;
        we_initiated_connection = true;
        gap_inquiry_start(30);
    }
}

inline void start_pairing_inquiry() {
    printf("[BT] No saved MAC. Starting search...\n");
    printf("Put DualSense in pairing mode:\n");
    printf("  Press PS + Create for 3 seconds\n");
    printf("  Light should blink rapidly\n\n");

    is_pairing = true;
    we_initiated_connection = true;
    gap_connectable_control(1);
    gap_inquiry_start(30);
}

inline void l2cap_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    if (packet_type == L2CAP_DATA_PACKET) {
        using namespace policy_device;
        if (size > 11 && response_report == 0) {
            response_report = 1;

        } else if (size > 11 && response_report == 1) {
            auto& registry = get_instance();
            if (ISonyGamepad* gamepad = registry.GetLibrary(0)) {
                FDeviceContext* context = gamepad->GetMutableDeviceContext();
                memcpy(context->Buffer, &packet[1], 78);
            }
        }
        return;
    }

    switch (hci_event_packet_get_type(packet)) {
        case L2CAP_EVENT_CHANNEL_OPENED: {
            uint16_t out_mtu = l2cap_event_channel_opened_get_remote_mtu(packet);
            printf("[L2CAP] Open channel. MTU: %u\n", out_mtu);

            uint8_t status = l2cap_event_channel_opened_get_status(packet);
            uint16_t psm = l2cap_event_channel_opened_get_psm(packet);
            uint16_t cid = l2cap_event_channel_opened_get_local_cid(packet);

            if (status != ERROR_CODE_SUCCESS) {
                printf("[L2CAP] Error PSM 0x%04x: 0x%02x\n", psm, status);
                return;
            }

            bd_addr_t addr;
            l2cap_event_channel_opened_get_address(packet, addr);
            printf("[L2CAP] Open channel! CID: 0x%04x, PSM: 0x%04x, Addr: %s\n",
                   cid, psm, bd_addr_to_str(addr));

            if (psm == PSM_HID_CONTROL) {
                if (l2cap_cid_interrupt == 0) {
                    printf("[L2CAP] HID Control connected. Opening HID Interrupt...\n");
                    l2cap_create_channel(&l2cap_packet_handler, addr, PSM_HID_INTERRUPT, 0xffff, &l2cap_cid_interrupt);
                }
            } else if (psm == PSM_HID_INTERRUPT) {
                printf("[L2CAP] HID Interrupt connected.\n");
                printf("========================================\n");
                printf("   DualSense READY TO USE!\n");
                printf("========================================\n");

                uint8_t get_feature[41] = {
                    0x43,
                    0x05
                };
                l2cap_send(l2cap_cid_control, get_feature, 41);
            }
            break;
        }
        case L2CAP_EVENT_CAN_SEND_NOW: {
            uint8_t buff[78] = {0};
            buff[0] = 0xa1;
            buff[1] = 0x31;
            buff[2] = 0x02;

            auto cod = l2cap_send(l2cap_cid_interrupt, buff, 79);
            switch (cod) {
                case L2CAP_DATA_LEN_EXCEEDS_REMOTE_MTU:
                    printf("L2CAP_DATA_LEN_EXCEEDS_REMOTE_MTU!\n");
                    break;
                case BTSTACK_ACL_BUFFERS_FULL:
                    printf("BTSTACK_ACL_BUFFERS_FULL!\n");
                    break;
                default: printf("error_or_success l2cap_send %02X \n", cod);
            };
            break;
        }

        case L2CAP_EVENT_CHANNEL_CLOSED: {
            uint16_t cid = l2cap_event_channel_closed_get_local_cid(packet);
            printf("[L2CAP] Close Channel 0x%04x\n", cid);

            if (cid == l2cap_cid_control) l2cap_cid_control = 0;
            if (cid == l2cap_cid_interrupt) l2cap_cid_interrupt = 0;
            if (cid == l2cap_cid_out_interrupt) l2cap_cid_out_interrupt = 0;
            break;
        }

        default:
            break;
    }
}

inline void hci_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    if (packet_type != HCI_EVENT_PACKET) return;

    switch (hci_event_packet_get_type(packet)) {
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
                printf("[BT] Bluetooth Stack active!\n");

                bd_addr_t saved_mac;
                link_key_t saved_key;
                if (flash_load_config(saved_mac, saved_key) && is_link_key_valid(saved_key)) {
                    printf("[BT] Paired device found: %s\n", bd_addr_to_str(saved_mac));
                    printf("[BT] Waiting for controller connection...\n");
                    bd_addr_copy(current_device_addr, saved_mac);
                    we_initiated_connection = false;  // We will wait for controller to connect
                    gap_connectable_control(1);
                    gap_discoverable_control(0);  // No need to be discoverable, already paired
                } else {
                    start_pairing_inquiry();
                }
            }
            break;

        // === DISCOVERY ===
        case HCI_EVENT_INQUIRY_RESULT:
        case HCI_EVENT_INQUIRY_RESULT_WITH_RSSI:
        case HCI_EVENT_EXTENDED_INQUIRY_RESPONSE: {
            bd_addr_t addr;
            uint32_t cod;
            uint8_t event_type = hci_event_packet_get_type(packet);

            if (event_type == HCI_EVENT_INQUIRY_RESULT) {
                cod = hci_event_inquiry_result_get_class_of_device(packet);
                hci_event_inquiry_result_get_bd_addr(packet, addr);
            } else if (event_type == HCI_EVENT_INQUIRY_RESULT_WITH_RSSI) {
                cod = hci_event_inquiry_result_with_rssi_get_class_of_device(packet);
                hci_event_inquiry_result_with_rssi_get_bd_addr(packet, addr);
            } else {
                cod = hci_event_extended_inquiry_response_get_class_of_device(packet);
                hci_event_extended_inquiry_response_get_bd_addr(packet, addr);
            }

            // CoD 0x002508 = Gamepad (Major: Peripheral, Minor: Gamepad)
            if ((cod & 0x000F00) == 0x000500) {
                printf("[HCI] Gamepad found: %s (CoD: 0x%06x)\n", bd_addr_to_str(addr), (unsigned int)cod);
                bd_addr_copy(current_device_addr, addr);
                device_found = true;
                gap_inquiry_stop();
            }
            break;
        }

        case GAP_EVENT_INQUIRY_COMPLETE:
            printf("[HCI] Search completed.\n");
            if (device_found) {
                device_found = false;
                we_initiated_connection = true;  // WE will initiate the connection
                printf("[HCI] Connecting to %s...\n", bd_addr_to_str(current_device_addr));
                hci_send_cmd(&hci_create_connection, current_device_addr,
                             hci_usable_acl_packet_types(), 0, 0, 0, 1);
            }
            break;

        // === CONNECTION ===
        case HCI_EVENT_CONNECTION_REQUEST: {
            bd_addr_t addr;
            hci_event_connection_request_get_bd_addr(packet, addr);
            uint32_t cod = hci_event_connection_request_get_class_of_device(packet);

            printf("[HCI] Connection request from %s (CoD: 0x%06x)\n",
                   bd_addr_to_str(addr), (unsigned int)cod);

            // Automatically accept gamepad connections
            if ((cod & 0x000F00) == 0x000500) {
                bd_addr_copy(current_device_addr, addr);
                we_initiated_connection = false;  // CONTROLLER initiated connection
                gap_inquiry_stop();  // Stop any search in progress
            }
            break;
        }

        case HCI_EVENT_CONNECTION_COMPLETE: {
            uint8_t status = hci_event_connection_complete_get_status(packet);
            bd_addr_t addr;
            hci_event_connection_complete_get_bd_addr(packet, addr);

            if (status == ERROR_CODE_SUCCESS) {
                hci_con_handle_t handle = hci_event_connection_complete_get_connection_handle(packet);
                printf("[HCI] ACL Connection established with %s (handle: 0x%04x)\n",
                       bd_addr_to_str(addr), handle);
                printf("[HCI] Initiator: %s\n", we_initiated_connection ? "WE" : "CONTROLLER");
                bd_addr_copy(current_device_addr, addr);

                printf("[HCI] Requesting authentication...\n");
                gap_request_security_level(handle, LEVEL_2);
            } else {
                printf("[HCI] Connection failed: 0x%02x\n", status);
                enable_discovery();
            }
            break;
        }

        // === PAIRING / AUTHENTICATION ===
        case HCI_EVENT_LINK_KEY_REQUEST: {
            bd_addr_t addr;
            hci_event_link_key_request_get_bd_addr(packet, addr);
            printf("[HCI] Link Key requested for %s\n", bd_addr_to_str(addr));

            bd_addr_t saved_mac;
            uint8_t saved_key[16];

            if (flash_load_config(saved_mac, saved_key) &&
                bd_addr_cmp(saved_mac, addr) == 0 &&
                is_link_key_valid(saved_key)) {
                printf("[HCI] Valid Link Key found! Sending...\n");
                link_key_used = true;  // Mark that we tried to use the key
                hci_send_cmd(&hci_link_key_request_reply, addr, saved_key);
            } else {
                printf("[HCI] No valid Link Key. Requesting pairing...\n");
                link_key_used = false;
                hci_send_cmd(&hci_link_key_request_negative_reply, addr);
            }
            break;
        }

        case HCI_EVENT_LINK_KEY_NOTIFICATION: {
            bd_addr_t addr;
            hci_event_link_key_request_get_bd_addr(packet, addr);

            // Link key is at offset 8, size 16
            uint8_t new_key[16];
            memcpy(new_key, &packet[8], 16);

            if (is_link_key_valid(new_key)) {
                printf("[HCI] New Link Key received for %s\n", bd_addr_to_str(addr));
                printf("[HCI] Key: ");
                for (int i = 0; i < 16; i++) printf("%02X", new_key[i]);
                printf("\n");

                flash_save_config(addr, new_key);
                bd_addr_copy(current_device_addr, addr);
            } else {
                printf("[HCI] WARNING: zero Link Key received, ignoring.\n");
            }
            break;
        }

        case HCI_EVENT_USER_CONFIRMATION_REQUEST: {
            bd_addr_t addr;
            hci_event_user_confirmation_request_get_bd_addr(packet, addr);
            printf("[HCI] SSP: Confirming pairing with %s\n", bd_addr_to_str(addr));
            hci_send_cmd(&hci_user_confirmation_request_reply, addr);
            break;
        }

        case HCI_EVENT_USER_PASSKEY_REQUEST: {
            bd_addr_t addr;
            hci_event_user_passkey_request_get_bd_addr(packet, addr);
            printf("[HCI] Passkey requested. Sending 0...\n");
            hci_send_cmd(&hci_user_passkey_request_reply, addr, 0);
            break;
        }

        case HCI_EVENT_PIN_CODE_REQUEST: {
            bd_addr_t addr;
            hci_event_pin_code_request_get_bd_addr(packet, addr);
            printf("[HCI] PIN requested. Sending 0002...\n");
            hci_send_cmd(&hci_pin_code_request_reply, addr, 4, "0000");
            break;
        }

        // === ENCRYPTION ===
        case HCI_EVENT_ENCRYPTION_CHANGE: {
            uint8_t status = hci_event_encryption_change_get_status(packet);
            uint8_t enabled = hci_event_encryption_change_get_encryption_enabled(packet);

            if (status == ERROR_CODE_SUCCESS && enabled) {
                printf("[HCI] Encryption ACTIVATED!\n");

                // ALWAYS open L2CAP after encryption - DualSense expects it
                if (l2cap_cid_control == 0) {
                    printf("[L2CAP] Opening HID Control channel...\n");
                    l2cap_create_channel(&l2cap_packet_handler, current_device_addr,
                                        PSM_HID_CONTROL, 0xffff, &l2cap_cid_control);
                }
            } else if (status != ERROR_CODE_SUCCESS) {
                printf("[HCI] Encryption error: 0x%02x\n", status);
            }
            break;
        }

        // === INCOMING L2CAP CONNECTION ===
        case L2CAP_EVENT_INCOMING_CONNECTION: {
            uint16_t local_cid = l2cap_event_incoming_connection_get_local_cid(packet);
            uint16_t psm = l2cap_event_incoming_connection_get_psm(packet);
            printf("[L2CAP] Incoming connection PSM: 0x%04x, CID: 0x%04x. Accepting...\n", psm, local_cid);
            l2cap_accept_connection(local_cid);
            break;
        }

        // === DISCONNECTION ===
        case HCI_EVENT_DISCONNECTION_COMPLETE: {
            uint8_t reason = packet[5];
            printf("[HCI] Disconnected. Reason: 0x%02x\n", reason);
            reset_connection_state();
            enable_discovery();
            break;
        }

        case HCI_EVENT_COMMAND_STATUS: {
            uint8_t status = hci_event_command_status_get_status(packet);
            if (status != 0) {
                uint16_t opcode = hci_event_command_status_get_command_opcode(packet);
                printf("[HCI] Error in command 0x%04x: 0x%02x\n", opcode, status);
            }
            break;
        }

        case HCI_EVENT_AUTHENTICATION_COMPLETE: {
            uint8_t status = hci_event_authentication_complete_get_status(packet);

            if (status != ERROR_CODE_SUCCESS) {
                printf("[HCI] Authentication failed: 0x%02x\n", status);

                // If we used a link key and it failed, delete it
                if (link_key_used) {
                    printf("[HCI] Link Key rejected by device. Deleting...\n");
                    flash_clear_config();
                    auth_failure_count++;
                }
            } else {
                printf("[HCI] Authentication successful!\n");
                auth_failure_count = 0;  // Reset counter
            }
            break;
        }

        default:
            break;
    }
}

static void init_bluetooth() {
    printf("[BT] Initializing Bluetooth Stack...\n");
    reset_connection_state();
    l2cap_init();

    // SSP (Secure Simple Pairing)
    gap_ssp_set_enable(true);
    gap_secure_connections_enable(true);
    gap_ssp_set_io_capability(SSP_IO_CAPABILITY_DISPLAY_YES_NO);
    gap_ssp_set_authentication_requirement(SSP_IO_AUTHREQ_MITM_PROTECTION_NOT_REQUIRED_GENERAL_BONDING);

    // Register L2CAP services to accept incoming connections
    // l2cap_register_service(&l2cap_packet_handler, PSM_HID_INTERRUPT, 150, LEVEL_2);
    // l2cap_register_service(&l2cap_packet_handler, PSM_HID_CONTROL, 150, LEVEL_2);

    // Allow connections
    gap_connectable_control(1);
    gap_discoverable_control(1);

    // Register handlers
    hci_event_callback.callback = &hci_packet_handler;
    hci_add_event_handler(&hci_event_callback);

    l2cap_event_callback.callback = &l2cap_packet_handler;
    l2cap_add_event_handler(&l2cap_event_callback);

    printf("[BT] Turning on radio...\n");
    gap_set_allow_role_switch(HCI_ROLE_MASTER);
    hci_power_control(HCI_POWER_ON);
}