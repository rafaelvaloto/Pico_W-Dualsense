#pragma once
#include "GCore/Templates/TGenericHardwareInfo.h"

uint32_t dualsense_crc32(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

struct pico_w_platform_policy {
    static void Read(FDeviceContext* Context) {
        printf("Reading from device...\n");
        (void)Context;
    }



    static void Write(FDeviceContext* Context) {
        if (!Context) return;
        printf("Writing to device...\n");
        // uint8_t* data = Context->GetRawOutputBuffer();

        uint8_t get_feature[41] = {
            0x43,
            0x05
        };

        l2cap_send(l2cap_cid_control, get_feature, 41);
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
