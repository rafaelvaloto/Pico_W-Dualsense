
#include <cstdio>
#include <string>
#include <vector>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "btstack.h"

// GamepadCore headers
#include "GCore/Interfaces/IPlatformHardwareInfo.h"
#include "GCore/Templates/TBasicDeviceRegistry.h"
#include "pico_w_platform.h"

extern "C" void sm_init(void);

void PrintControlsHelper()
{
    printf("\n=======================================================\n");
    printf("           DUALSENSE PICO W INTEGRATION                \n");
    printf("=======================================================\n");
    printf(" [ FACE BUTTONS ]\n");
    printf("   (X) Cross    : Heavy Rumble + RED Light\n");
    printf("   (O) Circle   : Soft Rumble  + BLUE Light\n");
    printf("   [ ] Square   : Trigger Effect: GAMECUBE (R2)\n");
    printf("   /_\\ Triangle : Stop All\n");
    printf("-------------------------------------------------------\n");
    printf(" [ D-PADS & SHOULDERS ]\n");
    printf("   [L1]    : Trigger Effect: Gallop (L2)\n");
    printf("   [R1]    : Trigger Effect: Machine (R2)\n");
    printf("   [UP]    : Trigger Effect: Feedback (Rigid)\n");
    printf("   [DOWN]  : Trigger Effect: Bow (Tension)\n");
    printf("   [LEFT]  : Trigger Effect: Weapon (Semi)\n");
    printf("   [RIGHT] : Trigger Effect: Automatic Gun (Buzz)\n");
    printf("=======================================================\n");
    printf(" Waiting for input...\n\n");
}

int main() {
    stdio_init_all();
    sleep_ms(2000); // Wait for USB serial

    printf("\n");
    printf("========================================\n");
    printf("   PICO W - DESCOBERTA BLUETOOTH\n");
    printf("========================================\n");

    // Initialize CYW43 (Bluetooth/WiFi chip)
    if (cyw43_arch_init()) {
        printf("ERROR: Failed to initialize CYW43\n");
        return -1;
    }
    printf("Hardware initialized OK\n");

    // Register hardware implementation for GamepadCore
    auto HardwareInfo = std::make_unique<HardwarePlatform::PicoW_Platform>();
    if (!HardwarePlatform::Registry) {
        HardwarePlatform::Registry = std::make_unique<PicoW_DeviceRegistry::PicoW_Registry>();
    }
    IPlatformHardwareInfo::SetInstance(std::move(HardwareInfo));

    HardwarePlatform::init_bluetooth();
    HardwarePlatform::Registry->PlugAndPlay(2.f);
    stdio_init_all();

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    sleep_ms(500);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    PrintControlsHelper();
    printf("looping OK\n");

    while(true) {}
    printf("saiu do loop...");
    return 0;
}
