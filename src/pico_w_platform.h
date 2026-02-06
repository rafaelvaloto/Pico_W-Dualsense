#pragma once
#include "GCore/Templates/TGenericHardwareInfo.h"


struct pico_w_platform_policy {
    static void Read(FDeviceContext* Context) {
        printf("Reading from device...\n");
        (void)Context;
    }

    static void Write(FDeviceContext* Context) {
        if (!Context) return;
        printf("Writing to device...\n");
        uint8_t* data = Context->GetRawOutputBuffer();
        if (data[0] == 0x31 && l2cap_cid_control != 0) {
            l2cap_send(l2cap_cid_control, data, 78);
        }
    }

    static bool CreateHandle(FDeviceContext* Context) {
        printf("Create handle from device...\n");
        (void)Context;
        return true;
    }

    static void InvalidateHandle(FDeviceContext* Context) {
        printf("Invalidate handle from device...\n");
        (void)Context;
    }

    static void Detect(std::vector<FDeviceContext>& Devices) {
        (void)Devices;
    }

    static void ProcessAudioHaptic(FDeviceContext* Context) {
        (void)Context;
    }

    static void InitializeAudioDevice(FDeviceContext* Context) {
        (void)Context;
    }
};
