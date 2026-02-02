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
    inline int TargetDeviceId = 0;
    static uint16_t l2cap_cid_control = 0;
    static uint16_t l2cap_cid_interrupt = 0;
    static std::unique_ptr<PicoW_DeviceRegistry::PicoW_Registry> Registry = nullptr;

    struct PicoW_PlatformPolicy {
        static void Read(FDeviceContext* Context) {}

        static void Write(FDeviceContext* Context) {
            if (!Context || Context->Handle == nullptr) return;

            if (l2cap_cid_interrupt == 0) {
                printf("Write: Interrupt Channel not initialized yet!\n");
                return;
            }

            const uint8_t* data = Context->GetRawOutputBuffer();
            printf("Initialize device\n");
            printf("Buffer: %02X, %02X, %02X, %02X \n", data[0], data[1], data[2], data[3]);

            switch (l2cap_send(l2cap_cid_interrupt, data, 78)) {
                case ERROR_CODE_SUCCESS:
                    printf("Pico W: Device Initializa, Success send l2cap_send...\n");
                    break;

                case BTSTACK_ACL_BUFFERS_FULL:
                    printf("Pico W: Queue full, waiting for buffer to release...\n");
                    break;

                case L2CAP_DATA_LEN_EXCEEDS_REMOTE_MTU:
                    printf("Error: Package larger than the MTU accepted by the controller!\n");
                    break;

                case L2CAP_LOCAL_CID_DOES_NOT_EXIST:
                    printf("Error: Non-existent L2CAP channel. Did the controller disconnect?\n");
                    break;

                case ERROR_CODE_COMMAND_DISALLOWED:
                    printf("Error: Command not allowed in current state.\n");
                    break;

                case ERROR_CODE_PARAMETER_OUT_OF_MANDATORY_RANGE:
                    printf("Error: Invalid send parameters.\n");
                    break;

                default:
                    printf("Unknown Bluetooth Error: \n");
                    break;
            }
        }

        static void Detect(std::vector<FDeviceContext>& Devices) {
            FDeviceContext NewDeviceContext;
            NewDeviceContext.Path = "PICO_W_DUALSENSE_0";
            NewDeviceContext.DeviceType = EDSDeviceType::DualSense;
            NewDeviceContext.IsConnected = true;
            NewDeviceContext.ConnectionType = EDSDeviceConnection::Bluetooth;
            NewDeviceContext.Handle = reinterpret_cast<void*>(1);

            Devices.push_back(NewDeviceContext);
            printf("DEBUG: [Pico_W] Device added to detection vector\n");
        }

        static bool CreateHandle(FDeviceContext* Context) {
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

        static void send_initial_output(uint16_t cid) {
            printf("Initial output report (0x31) sent to CID 0x%04x\n", cid);
        }

        static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
            if (packet_type == L2CAP_DATA_PACKET) {
                if (channel == l2cap_cid_interrupt) {
                    auto* Gamepad = Registry->GetLibrary(0);
                    if (Gamepad) {
                        FDeviceContext* Context = Gamepad->GetMutableDeviceContext();
                        memcpy(Context->Buffer, packet, size > 78 ? 78 : size);
                    }
                }
            }

            switch (hci_event_packet_get_type(packet)) {
                case BTSTACK_EVENT_STATE:
                    if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
                        printf("\n=== BLUETOOTH ===\n");
                        printf("<- ready to use ->\n");
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
                    break;

                case GAP_EVENT_INQUIRY_RESULT: {
                    bd_addr_t addr;
                    gap_event_inquiry_result_get_bd_addr(packet, addr);
                    const uint32_t device = gap_event_inquiry_result_get_class_of_device(packet);

                    printf("Device Address: %s CoD: 0x%06X \n", bd_addr_to_str(addr), device);

                    // gamepad (Peripheral)
                    if ((device & 0xFF00) == 0x0500) {
                        printf(" <- DEVICE FOUND! -> \n");
                        flash_alloc_macaddress(addr);
                        gap_inquiry_stop();
                    } else {
                        printf(" <- Pairing... -> \n");
                        printf(" <- Wait for it to reconnect. -> \n");
                    }
                } break;

                case GAP_EVENT_INQUIRY_COMPLETE: {
                    printf("Search complete.\n");
                    printf(" <- Connecting Interrupt Channel...! -> \n");
                    bd_addr_t macaddress;
                    flash_read_macaddress(macaddress);
                    printf("MAC Address: %s\n", bd_addr_to_str(macaddress));
                    uint8_t status_int = l2cap_create_channel(&packet_handler, macaddress, 0x13, 0xffff, &l2cap_cid_interrupt);
                    if (status_int != ERROR_CODE_SUCCESS) {
                        printf("L2CAP Create Channel 0x13 failed with status 0x%02x\n", status_int);
                    }

                    if (macaddress[0] == 0xFF) {
                        gap_connectable_control(1);
                        gap_inquiry_periodic_start(10, 16, 20);
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

                    if (psm == 0x13) {
                        printf("INTERRUPT Channel (0x13) opened. CID: 0x%04x\n", cid);
                        l2cap_cid_interrupt = cid;

                        bd_addr_t remote_addr;
                        memcpy(remote_addr, address, 6);
                        uint8_t status_ctrl = l2cap_create_channel(&packet_handler, remote_addr, 0x11, 0xffff, &l2cap_cid_control);
                        if (status_ctrl != ERROR_CODE_SUCCESS) {
                             printf("L2CAP Create Channel 0x11 failed with status 0x%02x\n", status_ctrl);
                        }
                    }
                    else if (psm == 0x11) {
                        printf("CONTROL Channel (0x11) opened. CID: 0x%04x\n", cid);
                        l2cap_cid_control = cid;
                        printf("=== DUALSENSE INTEGRATED SUCCESSFULLY! ===\n");
                    }
                } break;

                case L2CAP_EVENT_CHANNEL_CLOSED:
                        printf("L2CAP Channel closed. CID 0x%04x\n", l2cap_event_channel_closed_get_local_cid(packet));
                        gap_connectable_control(1);
                        gap_inquiry_periodic_start(10, 16, 20);
                    break;
            }
        }
    };

    static void init_bluetooth() {
        l2cap_init();
        gap_ssp_set_enable(1);
        gap_set_security_level(LEVEL_2);

        hci_event_callback.callback = &PicoW_PlatformPolicy::packet_handler;
        hci_add_event_handler(&hci_event_callback);

        // Turn on Bluetooth
        hci_power_control(HCI_POWER_ON);
    }

    using PicoW_Platform = GamepadCore::TGenericHardwareInfo<PicoW_PlatformPolicy>;
}

