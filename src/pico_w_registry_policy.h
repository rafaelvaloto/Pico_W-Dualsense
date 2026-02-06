#pragma once
#include "GCore/Templates/TGenericHardwareInfo.h"

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
