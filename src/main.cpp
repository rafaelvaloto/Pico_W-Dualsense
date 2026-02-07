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

inline void initialize_device() {
    printf("Initializing device...\n");
    FDeviceContext Context = {};
    Context.Path = "Bluetooth";
    Context.IsConnected = false;
    Context.DeviceType = EDSDeviceType::DualSense;
    Context.ConnectionType = EDSDeviceConnection::Bluetooth;

    auto& registry = policy_device::get_instance();
    registry.CreateDevice(Context);
}

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
            if (gamepad->IsConnected()) {
                gamepad->EnableTouch(true);
                gamepad->EnableMotionSensor(false); // enable sensors, gyro and accelerometer, can be used to control mouse cursor or for motion controls in games

                gamepad->UpdateInput(0.016f); // Update input state, should be called every frame with the time delta since last call
                FInputContext* input = gamepad->GetMutableDeviceContext()->GetInputState(); // Get the current input state of the controller, including button presses, analog stick positions, trigger values, touchpad state, etc.
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
                }  else if (input->bLeftShoulder) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    printf("L1 button pressed:\n");
                } else if (input->bRightShoulder) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    printf("R1 button pressed\n");
                } else if (input->bLeftStick) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    printf("L3 button pressed\n");
                } else if (input->bRightStick) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    printf("R3 button pressed\n");
                } else if (input->LeftTriggerAnalog > 0.0f) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    printf("L2: %f\n", input->LeftTriggerAnalog);
                } else if (input->RightTriggerAnalog > 0.0f) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    printf("R2: %f\n", input->RightTriggerAnalog);
                } else if (abs(input->LeftAnalog.X) > 0.1f || abs(input->LeftAnalog.Y) > 0.1f) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    printf("Left Analog: X %f, Y %f \n", input->LeftAnalog.X, input->LeftAnalog.Y);
                } else if (abs(input->RightAnalog.X) > 0.1f || abs(input->RightAnalog.Y) > 0.1f) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    printf("Right Analog: X %f, Y %f \n", input->RightAnalog.X, input->RightAnalog.Y);
                } else if (input->bIsTouching) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    printf("Fringer count: %d \n", (int)input->TouchFingerCount);
                    printf("Touchpad: X %f, Y %f \n", input->TouchPosition.X, input->TouchPosition.Y);
                }

            }
        }
        sleep_ms(16);
    }
    return 0;
}
