
//
// Created by rafaelvaloto on 04/02/2026.
//
#pragma once

#include <cstdint>
#include <cstdio>

#include "bluetooth.h"
#include "btstack_event.h"
#include "l2cap.h"
#include "pico_w_flash_ptr.h"
#include "classic/hid_host.h"
#include "classic/sdp_server.h"
#include "GCore/Interfaces/ISonyGamepad.h"
#include "GCore/Interfaces/IDeviceRegistry.h"
#include <vector>

// Forward declarations
namespace GamepadCore {
    template<typename DeviceRegistryPolicy> class TBasicDeviceRegistry;
}
#include "pico_w_registry_policy.h"
using pico_registry = GamepadCore::TBasicDeviceRegistry<pico_w_registry_policy>;
extern std::unique_ptr<pico_registry> registry;

#include "GCore/Templates/TBasicDeviceRegistry.h"

// Estado da conexão
static uint16_t l2cap_cid_control = 0;
static uint16_t l2cap_cid_interrupt = 0;
static bd_addr_t current_device_addr;
static bool device_found = false;
static bool is_pairing = false;
static bool we_initiated_connection = false;
static bool link_key_used = false;       // Flag: tentamos usar uma link key salva
static uint8_t auth_failure_count = 0;   // Contador de falhas consecutivas
static btstack_packet_callback_registration_t hci_event_callback;
static btstack_packet_callback_registration_t l2cap_event_callback;

// Helper: verifica se link key é válida (não zerada)
inline bool is_link_key_valid(const uint8_t *key) {
    for (int i = 0; i < 16; i++) {
        if (key[i] != 0) return true;
    }
    return false;
}

inline void reset_connection_state() {
    l2cap_cid_control = 0;
    l2cap_cid_interrupt = 0;
    is_pairing = false;
    we_initiated_connection = false;
    link_key_used = false;
    memset(current_device_addr, 0, sizeof(current_device_addr));
}

inline void enable_discovery() {
    printf("[BT] Habilitando discovery...\n");
    gap_connectable_control(1);
    gap_discoverable_control(1);

    // Verifica se já tem dispositivo pareado
    bd_addr_t saved_mac;
    link_key_t saved_key;
    if (flash_load_config(saved_mac, saved_key) && is_link_key_valid(saved_key)) {
        // Já tem pareamento - apenas aguarda conexão do controle
        printf("[BT] Aguardando reconexão de %s...\n", bd_addr_to_str(saved_mac));
        bd_addr_copy(current_device_addr, saved_mac);
    } else {
        // Não tem pareamento - inicia busca ativa
        printf("[BT] Iniciando busca por novos dispositivos...\n");
        is_pairing = true;
        we_initiated_connection = true;
        gap_inquiry_start(30);
    }
}

inline void start_pairing_inquiry() {
    printf("[BT] Nenhum MAC salvo. Iniciando busca...\n");
    printf("Coloque o DualSense em modo pareamento:\n");
    printf("  Pressione PS + Create por 3 segundos\n");
    printf("  A luz deve piscar rapidamente\n\n");

    is_pairing = true;
    we_initiated_connection = true;
    gap_connectable_control(1);
    gap_inquiry_start(30);
}

inline void l2cap_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    if (packet_type == L2CAP_DATA_PACKET) {
        if (size > 11) {
            printf("[L2CAP] Dados recebidos! CID: 0x%04x, Tamanho: %d\n", channel, size);
        }

        // TODO: processar dados HID aqui
        return;
    }

    if (packet_type != HCI_EVENT_PACKET) return;

    switch (hci_event_packet_get_type(packet)) {
        case L2CAP_EVENT_CHANNEL_OPENED: {
            uint16_t out_mtu = l2cap_event_channel_opened_get_remote_mtu(packet);
            printf("[L2CAP] Canal aberto. MTU de saída: %u\n", out_mtu);

            if (out_mtu < 78) {
                printf("[AVISO] MTU muito baixo para Report 0x31!\n");
            }

            uint8_t status = l2cap_event_channel_opened_get_status(packet);
            uint16_t psm = l2cap_event_channel_opened_get_psm(packet);
            uint16_t cid = l2cap_event_channel_opened_get_local_cid(packet);

            if (status != ERROR_CODE_SUCCESS) {
                printf("[L2CAP] Erro ao abrir canal PSM 0x%04x: 0x%02x\n", psm, status);
                return;
            }

            bd_addr_t addr;
            l2cap_event_channel_opened_get_address(packet, addr);
            printf("[L2CAP] Canal aberto! CID: 0x%04x, PSM: 0x%04x, Addr: %s\n",
                   cid, psm, bd_addr_to_str(addr));

            if (psm == PSM_HID_CONTROL) {
                l2cap_cid_control = cid;
                // SEMPRE abre Interrupt após Control
                if (l2cap_cid_interrupt == 0) {
                    printf("[L2CAP] HID Control conectado. Abrindo HID Interrupt...\n");
                    l2cap_create_channel(&l2cap_packet_handler, addr, PSM_HID_INTERRUPT, 0xffff, &l2cap_cid_interrupt);
                }
            } else if (psm == PSM_HID_INTERRUPT) {
                printf("[L2CAP] HID Interrupt conectado.\n");
                printf("========================================\n");
                printf("   DualSense PRONTO PARA USO!\n");
                printf("========================================\n");

                // Solicita report 0x31 inicializando a biblioteca
                if (registry) {
                    ISonyGamepad* Gamepad = registry->GetLibrary(0);
                    if (Gamepad) {
                        FDeviceContext* Context = Gamepad->GetMutableDeviceContext();
                        if (Context) {
                            printf("[BT] Enviando Report 0x31 de inicialização...\n");
                            Gamepad->UpdateOutput();
                        }
                    }
                }
            }
            break;
        }

        case L2CAP_EVENT_CHANNEL_CLOSED: {
            uint16_t cid = l2cap_event_channel_closed_get_local_cid(packet);
            printf("[L2CAP] Canal 0x%04x fechado\n", cid);

            if (cid == l2cap_cid_control) l2cap_cid_control = 0;
            if (cid == l2cap_cid_interrupt) l2cap_cid_interrupt = 0;
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
                printf("[BT] Stack Bluetooth ativo!\n");

                bd_addr_t saved_mac;
                link_key_t saved_key;
                if (flash_load_config(saved_mac, saved_key) && is_link_key_valid(saved_key)) {
                    printf("[BT] Dispositivo pareado encontrado: %s\n", bd_addr_to_str(saved_mac));
                    printf("[BT] Aguardando conexão do controle...\n");
                    bd_addr_copy(current_device_addr, saved_mac);
                    we_initiated_connection = false;  // Vamos esperar o controle conectar
                    gap_connectable_control(1);
                    gap_discoverable_control(0);  // Não precisa ser descoberto, já pareou
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
                printf("[HCI] Gamepad encontrado: %s (CoD: 0x%06x)\n", bd_addr_to_str(addr), (unsigned int)cod);
                bd_addr_copy(current_device_addr, addr);
                device_found = true;
                gap_inquiry_stop();
            }
            break;
        }

        case GAP_EVENT_INQUIRY_COMPLETE:
            printf("[HCI] Busca finalizada.\n");
            if (device_found) {
                device_found = false;
                we_initiated_connection = true;  // NÓS vamos iniciar a conexão
                printf("[HCI] Conectando a %s...\n", bd_addr_to_str(current_device_addr));
                hci_send_cmd(&hci_create_connection, current_device_addr,
                             hci_usable_acl_packet_types(), 0, 0, 0, 1);
            }
            break;

        // === CONEXÃO ===
        case HCI_EVENT_CONNECTION_REQUEST: {
            bd_addr_t addr;
            hci_event_connection_request_get_bd_addr(packet, addr);
            uint32_t cod = hci_event_connection_request_get_class_of_device(packet);

            printf("[HCI] Requisição de conexão de %s (CoD: 0x%06x)\n",
                   bd_addr_to_str(addr), (unsigned int)cod);

            // Aceita automaticamente conexões de gamepads
            if ((cod & 0x000F00) == 0x000500) {
                bd_addr_copy(current_device_addr, addr);
                we_initiated_connection = false;  // CONTROLE iniciou a conexão
                gap_inquiry_stop();  // Para qualquer busca em andamento
            }
            break;
        }

        case HCI_EVENT_CONNECTION_COMPLETE: {
            uint8_t status = hci_event_connection_complete_get_status(packet);
            bd_addr_t addr;
            hci_event_connection_complete_get_bd_addr(packet, addr);

            if (status == ERROR_CODE_SUCCESS) {
                hci_con_handle_t handle = hci_event_connection_complete_get_connection_handle(packet);
                printf("[HCI] Conexão ACL estabelecida com %s (handle: 0x%04x)\n",
                       bd_addr_to_str(addr), handle);
                printf("[HCI] Iniciador: %s\n", we_initiated_connection ? "NÓS" : "CONTROLE");
                bd_addr_copy(current_device_addr, addr);

                printf("[HCI] Solicitando autenticação...\n");
                gap_request_security_level(handle, LEVEL_2);
            } else {
                printf("[HCI] Falha na conexão: 0x%02x\n", status);
                enable_discovery();
            }
            break;
        }

        // === PAREAMENTO / AUTENTICAÇÃO ===
        case HCI_EVENT_LINK_KEY_REQUEST: {
            bd_addr_t addr;
            hci_event_link_key_request_get_bd_addr(packet, addr);
            printf("[HCI] Link Key solicitada para %s\n", bd_addr_to_str(addr));

            bd_addr_t saved_mac;
            uint8_t saved_key[16];

            if (flash_load_config(saved_mac, saved_key) &&
                bd_addr_cmp(saved_mac, addr) == 0 &&
                is_link_key_valid(saved_key)) {
                printf("[HCI] Link Key válida encontrada! Enviando...\n");
                link_key_used = true;  // Marca que tentamos usar a key
                hci_send_cmd(&hci_link_key_request_reply, addr, saved_key);
            } else {
                printf("[HCI] Sem Link Key válida. Solicitando pareamento...\n");
                link_key_used = false;
                hci_send_cmd(&hci_link_key_request_negative_reply, addr);
            }
            break;
        }

        case HCI_EVENT_LINK_KEY_NOTIFICATION: {
            bd_addr_t addr;
            hci_event_link_key_request_get_bd_addr(packet, addr);

            // Link key está no offset 8, tamanho 16
            uint8_t new_key[16];
            memcpy(new_key, &packet[8], 16);

            if (is_link_key_valid(new_key)) {
                printf("[HCI] Nova Link Key recebida para %s\n", bd_addr_to_str(addr));
                printf("[HCI] Key: ");
                for (int i = 0; i < 16; i++) printf("%02X", new_key[i]);
                printf("\n");

                flash_save_config(addr, new_key);
                bd_addr_copy(current_device_addr, addr);
            } else {
                printf("[HCI] AVISO: Link Key zerada recebida, ignorando.\n");
            }
            break;
        }

        case HCI_EVENT_USER_CONFIRMATION_REQUEST: {
            bd_addr_t addr;
            hci_event_user_confirmation_request_get_bd_addr(packet, addr);
            printf("[HCI] SSP: Confirmando pareamento com %s\n", bd_addr_to_str(addr));
            hci_send_cmd(&hci_user_confirmation_request_reply, addr);
            break;
        }

        case HCI_EVENT_USER_PASSKEY_REQUEST: {
            bd_addr_t addr;
            hci_event_user_passkey_request_get_bd_addr(packet, addr);
            printf("[HCI] Passkey solicitada. Enviando 0...\n");
            hci_send_cmd(&hci_user_passkey_request_reply, addr, 0);
            break;
        }

        case HCI_EVENT_PIN_CODE_REQUEST: {
            bd_addr_t addr;
            hci_event_pin_code_request_get_bd_addr(packet, addr);
            printf("[HCI] PIN solicitado. Enviando 0002...\n");
            hci_send_cmd(&hci_pin_code_request_reply, addr, 4, "0002");
            break;
        }

        // === CRIPTOGRAFIA ===
        case HCI_EVENT_ENCRYPTION_CHANGE: {
            uint8_t status = hci_event_encryption_change_get_status(packet);
            uint8_t enabled = hci_event_encryption_change_get_encryption_enabled(packet);

            if (status == ERROR_CODE_SUCCESS && enabled) {
                printf("[HCI] Criptografia ATIVADA!\n");

                // SEMPRE abre L2CAP após criptografia - DualSense espera isso
                if (l2cap_cid_control == 0) {
                    printf("[L2CAP] Abrindo canal HID Control...\n");
                    l2cap_create_channel(&l2cap_packet_handler, current_device_addr,
                                        PSM_HID_CONTROL, 0xffff, &l2cap_cid_control);
                }
            } else if (status != ERROR_CODE_SUCCESS) {
                printf("[HCI] Erro na criptografia: 0x%02x\n", status);
            }
            break;
        }

        // === CONEXÃO INCOMING L2CAP ===
        case L2CAP_EVENT_INCOMING_CONNECTION: {
            uint16_t local_cid = l2cap_event_incoming_connection_get_local_cid(packet);
            uint16_t psm = l2cap_event_incoming_connection_get_psm(packet);
            printf("[L2CAP] Conexão incoming PSM: 0x%04x, CID: 0x%04x. Aceitando...\n", psm, local_cid);
            l2cap_accept_connection(local_cid);
            break;
        }

        // === DESCONEXÃO ===
        case HCI_EVENT_DISCONNECTION_COMPLETE: {
            uint8_t reason = packet[5];
            printf("[HCI] Desconectado. Motivo: 0x%02x\n", reason);
            reset_connection_state();
            enable_discovery();
            break;
        }

        case HCI_EVENT_COMMAND_STATUS: {
            uint8_t status = hci_event_command_status_get_status(packet);
            if (status != 0) {
                uint16_t opcode = hci_event_command_status_get_command_opcode(packet);
                printf("[HCI] Erro no comando 0x%04x: 0x%02x\n", opcode, status);
            }
            break;
        }

        case HCI_EVENT_AUTHENTICATION_COMPLETE: {
            uint8_t status = hci_event_authentication_complete_get_status(packet);

            if (status != ERROR_CODE_SUCCESS) {
                printf("[HCI] Falha na autenticação: 0x%02x\n", status);

                // Se usamos uma link key e falhou, apaga
                if (link_key_used) {
                    printf("[HCI] Link Key rejeitada pelo dispositivo. Apagando...\n");
                    flash_clear_config();
                    auth_failure_count++;
                }
            } else {
                printf("[HCI] Autenticação bem sucedida!\n");
                auth_failure_count = 0;  // Reset contador
            }
            break;
        }

        default:
            break;
    }
}

static void init_bluetooth() {
    printf("[BT] Inicializando Bluetooth Stack...\n");

    reset_connection_state();
    l2cap_init();


    // SSP (Secure Simple Pairing)
    gap_ssp_set_enable(true);
    gap_secure_connections_enable(true);
    gap_ssp_set_io_capability(SSP_IO_CAPABILITY_DISPLAY_YES_NO);
    gap_ssp_set_authentication_requirement(SSP_IO_AUTHREQ_MITM_PROTECTION_NOT_REQUIRED_GENERAL_BONDING);

    // Registra serviços L2CAP para aceitar conexões incoming
    // l2cap_register_service(&l2cap_packet_handler, PSM_HID_CONTROL, 0xffff, LEVEL_4);
    // l2cap_register_service(&l2cap_packet_handler, PSM_HID_INTERRUPT, 0xffff, LEVEL_4);

    // Permite conexões
    gap_connectable_control(1);
    gap_discoverable_control(0);

    // Registra handlers
    hci_event_callback.callback = &hci_packet_handler;
    hci_add_event_handler(&hci_event_callback);

    l2cap_event_callback.callback = &l2cap_packet_handler;
    l2cap_add_event_handler(&l2cap_event_callback);

    printf("[BT] Ligando rádio...\n");
    gap_set_allow_role_switch(HCI_ROLE_MASTER);
    hci_power_control(HCI_POWER_ON);
}