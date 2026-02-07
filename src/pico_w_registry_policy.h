#pragma once
#include "GCore/Templates/TBasicDeviceRegistry.h"
#include "GCore/Types/DSCoreTypes.h"
#include "GCore/Templates/TGenericHardwareInfo.h"



namespace policy_device {

    struct pico_w_registry_policy {
        using EngineIdType = int32_t;

        struct Hasher {
            size_t operator()(const int32_t& v) const { return std::hash<int32_t>{}(v); }
        };

        static EngineIdType AllocEngineDevice() {
            return 0;
        }

        static void DisconnectDevice(EngineIdType id) {
            printf("[Policy] Disconnecting device %d\n", id);
        }

        static void DispatchNewGamepad(EngineIdType id) {
            printf("[Policy] New gamepad dispatched: %d\n", id);
        }
    };

    using pico_registry_device = GamepadCore::TBasicDeviceRegistry<pico_w_registry_policy>;
    static pico_registry_device& get_instance() {
        static pico_registry_device instance;
        return instance;
    }



}


static void initialize_device() {
    printf("Initializing device...\n");
    FDeviceContext Context = {};
    Context.Path = "Bluetooth";
    Context.IsConnected = true;
    Context.DeviceType = EDSDeviceType::DualSense;
    Context.ConnectionType = EDSDeviceConnection::Bluetooth;

    auto& registry = policy_device::get_instance();
    registry.CreateDevice(Context);
}
