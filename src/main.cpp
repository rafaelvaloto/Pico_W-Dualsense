
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

    printf("\n");
    printf("========================================\n");
    printf("   PICO W - BLUETOOTH DISCOVERY\n");
    printf("========================================\n");

    // Initialize CYW43 (Bluetooth/WiFi chip)
    if (cyw43_arch_init()) {
        printf("ERROR: Failed to initialize CYW43\n");
        return -1;
    }


    printf("Hardware initialized OK\n");
    auto HardwareInfo = std::make_unique<HardwarePlatform::PicoW_Platform>();
    IPlatformHardwareInfo::SetInstance(std::move(HardwareInfo));
    if (!HardwarePlatform::Registry) {
        HardwarePlatform::Registry = std::make_unique<PicoW_DeviceRegistry::PicoW_Registry>();
    }

    PrintControlsHelper();
    HardwarePlatform::init_bluetooth();

    l2cap_init();
    hci_event_callback.callback = HardwarePlatform::PicoW_PlatformPolicy::packet_handler;
    hci_add_event_handler(&hci_event_callback);

    // Turn on Bluetooth
    hci_power_control(HCI_POWER_ON);
    int blink_cnt = 0;
    while(true) {
        if (HardwarePlatform::Registry) {
            HardwarePlatform::Registry->PlugAndPlay(0.01f);
            auto* Gamepad = HardwarePlatform::Registry->GetLibrary(0);
            if (Gamepad) {
                Gamepad->UpdateInput(0.01f);
                Gamepad->UpdateOutput();

                FDeviceContext* Context = Gamepad->GetMutableDeviceContext();
                if (FInputContext* Input = Context->GetInputState()) {
                    if (Input->bCross) {
                        printf("Cross pressed\n");
                    }
                }
            }
        }

        if (blink_cnt++ % 50 == 0) cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        if (blink_cnt % 50 == 25) cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

        sleep_ms(10);
    }
    // printf("saiu do loop...");
    return 0;
}
