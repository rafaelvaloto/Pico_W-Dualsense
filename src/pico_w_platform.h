#pragma once

#include <vector>
#include <cstdio>
#include <cstring>
#include <memory>
#include "GCore/Templates/TGenericHardwareInfo.h"
#include "pico_w_flash_ptr.h"
#include "btstack.h"


static btstack_packet_callback_registration_t hci_event_callback;
namespace PicoW_DeviceRegistry {

    struct PicoW_RegistryPolicy {
        using EngineIdType = int32_t;

        struct Hasher {
            size_t operator()(const int32_t& v) const { return std::hash<int32_t>{}(v); }
        };

        static EngineIdType AllocEngineDevice() {
            static EngineIdType NextId = 0;
            return NextId;
        }

        static void DisconnectDevice(EngineIdType id) {
            printf("[Policy] Disconnecting device %d\n", id);
        }

        static void DispatchNewGamepad(EngineIdType id) {
            printf("[Policy] New gamepad dispatched: %d\n", id);
        }
    };

    using PicoW_Registry = GamepadCore::TBasicDeviceRegistry<PicoW_RegistryPolicy>;
}

namespace HardwarePlatform {
    static uint16_t l2cap_cid_control = 0;
    static uint16_t l2cap_cid_interrupt = 0;
    static bd_addr_t connected_addr;
    static bool device_found = false;
    static uint16_t outgoing_control_cid = 0;
    static uint16_t outgoing_interrupt_cid = 0;
    static uint8_t pending_tx[78] = {};
    static bool tx_pending = false;

    static std::unique_ptr<PicoW_DeviceRegistry::PicoW_Registry> Registry = nullptr;

    struct PicoW_PlatformPolicy {
        static void Read(FDeviceContext* Context) {

        }

        static void Write(FDeviceContext* Context) {
            if (!Context || l2cap_cid_interrupt == 0) return;
            memcpy(pending_tx, Context->GetRawOutputBuffer(), 78);
            tx_pending = true;
            l2cap_request_can_send_now_event(l2cap_cid_interrupt);
        }

        static void Detect(std::vector<FDeviceContext>& Devices) {
            FDeviceContext NewDeviceContext;
            NewDeviceContext.Path = "PICO_W_DUALSENSE_0";
            NewDeviceContext.DeviceType = EDSDeviceType::DualSense;
            NewDeviceContext.IsConnected = true;
            NewDeviceContext.ConnectionType = EDSDeviceConnection::Bluetooth;
            Devices.push_back(NewDeviceContext);
        }

        static bool CreateHandle(FDeviceContext* Context) {
            Context->Handle = reinterpret_cast<void*>(1);
            return true;
        }

        static void InvalidateHandle(FDeviceContext* Context) {
            (void)Context;
        }

        static void ProcessAudioHaptic(FDeviceContext* Context) {
            (void)Context;
        }

        static void InitializeAudioDevice(FDeviceContext* Context) {
            (void)Context;
        }

        static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
            if (packet_type == L2CAP_DATA_PACKET) {
                packet[1] = 0x31;

                if (channel == l2cap_cid_interrupt && l2cap_cid_interrupt != 0) {
                    if (Registry) {
                        ISonyGamepad* Gamepad = Registry->GetLibrary(0);
                        if (Gamepad) {
                            FDeviceContext* Context = Gamepad->GetMutableDeviceContext();
                            memcpy(Context->Buffer, &packet[2], size - 2 > 78 ? 78 : size - 2);
                        }
                    }

                }
            }

            if (packet_type != HCI_EVENT_PACKET) return;

            switch (hci_event_packet_get_type(packet)) {
                case L2CAP_EVENT_CAN_SEND_NOW: {
                    uint16_t cid = l2cap_event_can_send_now_get_local_cid(packet);
                    if (tx_pending && cid == l2cap_cid_interrupt) {
                        uint8_t status = l2cap_send(cid, pending_tx, 78);
                        tx_pending = false;
                        if (status != ERROR_CODE_SUCCESS) {
                            if (status == BTSTACK_ACL_BUFFERS_FULL) {
                                tx_pending = true;
                                l2cap_request_can_send_now_event(cid);
                            } else {
                                printf("L2CAP Error: 0x%02x\n", status);
                            }
                        }
                    }
                } break;
                case BTSTACK_EVENT_STATE:
                    if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
                        printf("\n=== BLUETOOTH ===\n");
                        printf("<- ready to use ->\n");

                        // Register L2CAP Services for HID (to allow incoming connections)
                        l2cap_register_service(&packet_handler, PSM_HID_CONTROL, 0xffff, gap_get_security_level());
                        l2cap_register_service(&packet_handler, PSM_HID_INTERRUPT, 0xffff, gap_get_security_level());

                        bd_addr_t macaddress;
                        flash_read_macaddress(macaddress);
                        if (macaddress[0] != 0xFF || macaddress[1] != 0xFF) {
                            printf("Stored MAC found: %s. Connecting...\n", bd_addr_to_str(macaddress));
                            memcpy(connected_addr, macaddress, 6);
                            device_found = true;
                            uint8_t status = l2cap_create_channel(&packet_handler, connected_addr, PSM_HID_CONTROL, 0xffff, &l2cap_cid_control);
                            if (status) printf("L2CAP Control connection failed: 0x%02x\n", status);
                        } else {
                            printf("Put DualSense in pairing mode:\n");
                            printf("  Press PS + Create for 3 seconds\n");
                            printf("  The light should flash rapidly\n\n");
                            printf("Starting search...\n\n");

                            switch (gap_inquiry_start(30)) {
                                case 0:
                                    gap_connectable_control(0);
                                    break;
                                case ERROR_CODE_COMMAND_DISALLOWED:
                                case ERROR_CODE_INVALID_HCI_COMMAND_PARAMETERS:
                                    printf("\n\nERROR_CODE_COMMAND_DISALLOWED or ERROR_CODE_INVALID_HCI_COMMAND_PARAMETERS\n\n");
                                    gap_connectable_control(1);
                                    gap_inquiry_periodic_start(10, 16, 20);
                                    break;
                                default: printf("\n\ngap_inquiry_start unknown status\n\n");
                            }
                        }
                    }
                    break;

                case GAP_EVENT_INQUIRY_RESULT: {
                    bd_addr_t addr;
                    gap_event_inquiry_result_get_bd_addr(packet, addr);
                    const uint32_t device = gap_event_inquiry_result_get_class_of_device(packet);

                    printf("Device Address: %s CoD: 0x%06X \n", bd_addr_to_str(addr), (unsigned int)device);

                    if (((device & 0x1F00) == 0x0500) && ((device & 0xFF) == 0x08)) {
                        printf(" <- DUALSENSE FOUND! -> \n");
                        memcpy(connected_addr, addr, 6);
                        device_found = true;
                        flash_alloc_macaddress(addr);
                        gap_inquiry_stop();
                    }
                } break;

                case GAP_EVENT_INQUIRY_COMPLETE: {
                    printf("Query completed\n");
                    if (device_found) {
                        printf("Initiating connection to %s...\n", bd_addr_to_str(connected_addr));
                        uint8_t status = l2cap_create_channel(&packet_handler, connected_addr, PSM_HID_CONTROL, 0xffff, &l2cap_cid_control);
                        if (status) printf("L2CAP Control connection failed: 0x%02x\n", status);
                        device_found = false; // Reset for next time if needed
                    }
                } break;

                case L2CAP_EVENT_CHANNEL_OPENED: {
                    uint8_t status = l2cap_event_channel_opened_get_status(packet);
                    uint16_t psm = l2cap_event_channel_opened_get_psm(packet);
                    uint16_t cid = l2cap_event_channel_opened_get_local_cid(packet);

                    bd_addr_t address;
                    l2cap_event_channel_opened_get_address(packet, address);
                    if (status != ERROR_CODE_SUCCESS) {
                        printf("L2CAP ERROR: PSM 0x%02x failed (Status 0x%02x)\n", psm, status);
                        return;
                    }

                    if (psm == PSM_HID_CONTROL) {
                        printf("CONTROL Channel (0x11) opened. CID: 0x%04x\n", cid);
                        l2cap_cid_control = cid;

                        // If the connection was initiated by the controller, we don't need to create the interrupt channel manually?
                        // Actually, the controller usually opens both. But if we initiate, we open the second one.
                        // Let's check if we already have the remote address to try to open the second channel if necessary.
                        
                        bd_addr_t remote_addr;
                        memcpy(remote_addr, address, 6);
                        
                        // If the interrupt channel is not yet open, we try to open it.
                        if (l2cap_cid_interrupt == 0) {
                            printf(" <- Connecting Interrupt Channel (0x13)...! -> \n");
                            uint8_t status_int = l2cap_create_channel(&packet_handler, address, PSM_HID_INTERRUPT, 0xffff, &l2cap_cid_interrupt);
                            if (status_int != ERROR_CODE_SUCCESS) {
                                 printf("L2CAP Create Channel 0x13 failed with status 0x%02x\n", status_int);
                            }
                        }
                    }
                    else if (psm == PSM_HID_INTERRUPT) {
                        printf("INTERRUPT Channel (0x13) opened. CID: 0x%04x\n", cid);
                        l2cap_cid_interrupt = cid;
                        printf("=== DUALSENSE INTEGRATED SUCCESSFULLY! ===\n");
                    }
                } break;


                case HCI_EVENT_USER_CONFIRMATION_REQUEST:
                    printf("User Confirmation Request. Confirming...\n");
                    bd_addr_t conf_addr;
                    hci_event_user_confirmation_request_get_bd_addr(packet, conf_addr);
                    break;

                case L2CAP_EVENT_CHANNEL_CLOSED:
                        printf("L2CAP Channel closed. CID 0x%04x\n", l2cap_event_channel_closed_get_local_cid(packet));
                        l2cap_cid_control = 0;
                        l2cap_cid_interrupt = 0;
                        gap_connectable_control(1);
                        gap_inquiry_periodic_start(10, 16, 20);
                    break;
            }
        }
    };

    static void init_bluetooth() {
        gap_set_class_of_device(0x2508);
        gap_discoverable_control(1);
        gap_set_local_name("PicoW_Gamepad_Test");
    }

    using PicoW_Platform = GamepadCore::TGenericHardwareInfo<PicoW_PlatformPolicy>;
}

