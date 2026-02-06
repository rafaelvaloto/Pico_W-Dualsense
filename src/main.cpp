
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

// Forward declarations for btstack
#include <memory>
namespace GamepadCore {
    template<typename DeviceRegistryPolicy> class TBasicDeviceRegistry;
}
#include "pico_w_registry_policy.h"
using pico_registry = GamepadCore::TBasicDeviceRegistry<pico_w_registry_policy>;
std::unique_ptr<pico_registry> registry;

// GamepadCore headers
#include "pico_w_btstack.h"
#include "GCore/Interfaces/IPlatformHardwareInfo.h"
#include "GCore/Templates/TBasicDeviceRegistry.h"
#include "pico_w_platform.h"
#include "GCore/Interfaces/ISonyGamepad.h"

using pico_platform = GamepadCore::TGenericHardwareInfo<pico_w_platform_policy>;

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
    auto HardwareInfo = std::make_unique<pico_platform>();
    IPlatformHardwareInfo::SetInstance(std::move(HardwareInfo));

    printf("Registry devices initialized OK\n");
    registry = std::make_unique<pico_registry>();

    FDeviceContext Context = {};
    Context.ConnectionType = EDSDeviceConnection::Bluetooth;
    Context.IsConnected = true;
    Context.DeviceType = EDSDeviceType::DualSense;
    Context.Path = "Bluetooth";

    registry->CreateDevice(Context);
    sleep_ms(100);

    init_bluetooth();

    int blink_cnt = 0;
    bool is_running = true;
    while(is_running) {
        #if PICO_CYW43_ARCH_POLL
        cyw43_arch_poll();
        #endif

        if (blink_cnt++ % 50 == 0) cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        if (blink_cnt % 50 == 25) cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

        ISonyGamepad* Gamepad = registry->GetLibrary(0);
        if (Gamepad) {
            // Se o DualSense estiver pronto, processa entrada e saída
            if (l2cap_cid_interrupt != 0) {
                Gamepad->UpdateInput(0.016f);
            }
        } else {
            is_running = false;
        }
        sleep_ms(16);
    }

    return 0;
}
