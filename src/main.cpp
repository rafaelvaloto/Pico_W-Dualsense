#include "pico/cyw43_arch.h"

// Forward declarations for btstack
#include <memory>
#include "pico_w_registry_policy.h"

// GamepadCore headers
#include "pico_w_btstack.h"
#include "GCore/Interfaces/IPlatformHardwareInfo.h"
#include "GCore/Templates/TBasicDeviceRegistry.h"
#include "pico_w_platform.h"
#include "GCore/Interfaces/ISonyGamepad.h"
using pico_platform = GamepadCore::TGenericHardwareInfo<pico_w_platform_policy>;


int main() {
    stdio_init_all();
    if (cyw43_arch_init()) {
        printf("ERROR: Failed to initialize CYW43\n");
        return -1;
    }

    sleep_ms(2000);

    printf("\n");
    printf("========================================\n");
    printf("   PICO W - BLUETOOTH DISCOVERY\n");
    printf("========================================\n");

    auto HardwareInfo = std::make_unique<pico_platform>();
    IPlatformHardwareInfo::SetInstance(std::move(HardwareInfo));
    printf("Hardware initialized OK\n");

    using namespace policy_device;
    initialize_device();
    auto& registry = get_instance();
    printf("Device initialized OK\n");

    init_bluetooth();
    printf("Bluetooth initialized OK\n");

    int blink_cnt = 0;
    while(true) {
        if (blink_cnt++ % 50 == 0) cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        if (blink_cnt % 50 == 25) cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

        if (auto* gamepad = registry.GetLibrary(0)) {
            gamepad->UpdateInput(0.016f);
            FInputContext* input = gamepad->GetMutableDeviceContext()->GetInputState();
            if (input->bCross) {
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                printf("Cross button pressed\n");
            } else if (input->bCircle) {
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                printf("Circle button pressed\n");
            } else if (input->bSquare) {
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                printf("Square button pressed\n");
            } else if (input->bTriangle) {
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                printf("Triangle button pressed\n");
            }
        }
        sleep_ms(16);
    }
    return 0;
}
