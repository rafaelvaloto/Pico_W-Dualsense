#pragma once
#include "GCore/Templates/TGenericHardwareInfo.h"
#include "GImplementations/Utils/GamepadSensors.h"


struct pico_w_platform_policy {

    static void Write(FDeviceContext* Context) {
        if (!Context) return;
        printf("l2cap_request_can_send_now_event to device \n");
        // l2cap_request_can_send_now_event(l2cap_cid_out_interrupt);
    }

    static bool CreateHandle(FDeviceContext* Context) {
        return true;
    }

    static void ConfigureFeature(FDeviceContext* Context) {}
    static void Read(FDeviceContext* Context) {}
    static void Detect(std::vector<FDeviceContext>& Devices) {}
    static void InvalidateHandle(FDeviceContext* Context) {}
    static void ProcessAudioHaptic(FDeviceContext* Context) {}
    static void InitializeAudioDevice(FDeviceContext* Context) {}
};

