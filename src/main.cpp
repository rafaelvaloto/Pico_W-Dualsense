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

inline void print_controls_helper()
{
    printf("=======================================================\n");
    printf("           DUALSENSE INTEGRATION TEST                  \n");
    printf("=======================================================\n");
    printf(" [ FACE BUTTONS ]\n");
    printf("   (X) Cross    : Heavy Rumble + RED Light\n");
    printf("   (O) Circle   : Soft Rumble  + YELLOW Light\n");
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
    printf(" Waiting for input...\n");
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

    std::vector<uint8_t> BufferTrigger;
    BufferTrigger.resize(10);

    int blink_cnt = 0;
    int unique_send = 0;
    int reset_bt_send = 0;
    while(true) {

        if (auto* gamepad = registry.GetLibrary(0)) {
            if (gamepad->IsConnected()) {
                // enable touchpad and sensors, gyro and accelerometer, can be used to control mouse cursor or for motion controls in games
                gamepad->EnableTouch(true);
                gamepad->EnableMotionSensor(false);

                gamepad->UpdateInput(0.016f); // Update input state, should be called every frame with the time delta since last call

                FInputContext* input = gamepad->GetMutableDeviceContext()->GetInputState();
                if (input->bStart || input->bShare) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    if (reset_bt_send == 0) { // l2cap_send to set lightbar to white and vibrate
                        unique_send = 1;
                        reset_bt_send = 1;

                        printf("Resetting bluetooth features...\n");
                        gamepad->GetMutableDeviceContext()->Output.Feature = {0xFF, 0xFF, 0x00, 0x00};
                        gamepad->SetLightbar({0, 0, 0, 0});
                        gamepad->UpdateOutput();
                        sleep_ms(400);
                        gamepad->GetMutableDeviceContext()->Output.Feature = {0x57, 0xFF, 0x00, 0x00};
                        gamepad->SetLightbar({255, 255, 255, 0});
                        gamepad->SetPlayerLed(EDSPlayer::One, 0xff);
                        gamepad->UpdateOutput();
                        sleep_ms(400);
                    }

                    printf("Complete configuration features...\n");
                    print_controls_helper();
                } else if (input->bCross) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    printf("Cross button pressed\n");
                    if (unique_send == 0) {
                        unique_send = 1;
                        gamepad->SetLightbar({0xff, 0, 0, 0});
                        gamepad->SetVibration(100, 0);
                        gamepad->UpdateOutput();
                    }
                } else if (input->bCircle) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    printf("Circle button pressed\n");

                    if (unique_send == 0) {
                        unique_send = 1;
                        gamepad->SetLightbar({0xff, 0xff, 0, 0});
                        gamepad->SetVibration(0, 50);
                        gamepad->UpdateOutput();
                    }
                } else if (input->bSquare) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    printf("Square button pressed\n");
                    printf("Trigger R: GameCube (0x02)\n");
                    if (unique_send == 0) {
                        unique_send = 1;
                        gamepad->GetIGamepadTrigger()->SetGameCube(EDSGamepadHand::Right);
                        gamepad->UpdateOutput();
                    }
                } else if (input->bTriangle) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    printf("Triangle button pressed\n");

                    if (unique_send == 0) { // l2cap_send to set lightbar to white and vibrate
                        unique_send = 1;
                        gamepad->SetLightbar({0, 255, 0});
                        gamepad->GetIGamepadTrigger()->StopTrigger(EDSGamepadHand::AnyHand);
                        gamepad->UpdateOutput();
                    }
                }  else if (input->bLeftShoulder) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    printf("L1 button pressed:\n");

                    if (unique_send == 0) { // l2cap_send to set lightbar to white and vibrate
                        unique_send = 1;
                        BufferTrigger[0] = 0x23;
                        BufferTrigger[1] = 0x82;
                        BufferTrigger[2] = 0x00;
                        BufferTrigger[3] = 0xf7;
                        BufferTrigger[4] = 0x02;
                        BufferTrigger[5] = 0x00;
                        BufferTrigger[6] = 0x00;
                        BufferTrigger[7] = 0x00;
                        BufferTrigger[8] = 0x00;
                        BufferTrigger[9] = 0x00;

                        printf("Trigger L: Gallop (0x23)\n");
                        gamepad->GetIGamepadTrigger()->SetCustomTrigger(EDSGamepadHand::Left, BufferTrigger);
                        gamepad->UpdateOutput();
                    }
                } else if (input->bDpadLeft) {
                    if (unique_send == 0) { // l2cap_send to set lightbar to white and vibrate
                        unique_send = 1;
                        BufferTrigger[0] = 0x25;
                        BufferTrigger[1] = 0x08;
                        BufferTrigger[2] = 0x01;
                        BufferTrigger[3] = 0x07;
                        BufferTrigger[4] = 0x00;
                        BufferTrigger[5] = 0x00;
                        BufferTrigger[6] = 0x00;
                        BufferTrigger[7] = 0x00;
                        BufferTrigger[8] = 0x00;
                        BufferTrigger[9] = 0x00;
                        printf("Trigger R: Weapon (0x25)\n");

                        gamepad->GetIGamepadTrigger()->SetCustomTrigger(EDSGamepadHand::Right, BufferTrigger);
                        gamepad->UpdateOutput();
                    }
                } else if (input->bDpadRight) {
                    if (unique_send == 0) {
                        unique_send = 1;
                        printf("Trigger R: AutomaticGun (0x26)\n");
                        gamepad->GetIGamepadTrigger()->SetMachineGun26(0xed, 0x03, 0x02, 0x09, EDSGamepadHand::Right);
                        gamepad->UpdateOutput();
                    }
                } else if (input->bDpadDown) {
                    if (unique_send == 0) { // l2cap_send to set lightbar to white and vibrate
                        unique_send = 1;
                        BufferTrigger[0] = 0x22;
                        BufferTrigger[1] = 0x02;
                        BufferTrigger[2] = 0x01;
                        BufferTrigger[3] = 0x3f;
                        BufferTrigger[4] = 0x00;
                        BufferTrigger[5] = 0x00;
                        BufferTrigger[6] = 0x00;
                        BufferTrigger[7] = 0x00;
                        BufferTrigger[8] = 0x00;
                        BufferTrigger[9] = 0x00;
                        printf("Trigger R: Bow (0x22)\n");

                        gamepad->GetIGamepadTrigger()->SetCustomTrigger(EDSGamepadHand::Right, BufferTrigger);
                        gamepad->UpdateOutput();
                    }
                } else if (input->bDpadUp) {
                    if (unique_send == 0) { // l2cap_send to set lightbar to white and vibrate
                        unique_send = 1;
                        BufferTrigger[0] = 0x21;
                        BufferTrigger[1] = 0xfe;
                        BufferTrigger[2] = 0x03;
                        BufferTrigger[3] = 0xf8;
                        BufferTrigger[4] = 0xff;
                        BufferTrigger[5] = 0xff;
                        BufferTrigger[6] = 0x3f;
                        BufferTrigger[7] = 0x00;
                        BufferTrigger[8] = 0x00;
                        BufferTrigger[9] = 0x00;

                        printf("Trigger L: feedback (0x21)\n");

                        gamepad->GetIGamepadTrigger()->SetCustomTrigger(EDSGamepadHand::Left, BufferTrigger);
                        gamepad->UpdateOutput();
                    }
                } else if (input->bRightShoulder) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    printf("R1 button pressed\n");

                    if (unique_send == 0) { // l2cap_send to set lightbar to white and vibrate
                        unique_send = 1;
                        BufferTrigger[0] = 0x27;
                        BufferTrigger[1] = 0x80;
                        BufferTrigger[2] = 0x02;
                        BufferTrigger[3] = 0x3a;
                        BufferTrigger[4] = 0x0a;
                        BufferTrigger[5] = 0x04;
                        BufferTrigger[6] = 0x00;
                        BufferTrigger[7] = 0x00;
                        BufferTrigger[8] = 0x00;
                        BufferTrigger[9] = 0x00;

                        printf("Trigger R: Machine (0x27)\n");
                        gamepad->GetIGamepadTrigger()->SetCustomTrigger(EDSGamepadHand::Right, BufferTrigger);
                        gamepad->UpdateOutput();
                    }
                } else if (input->bLeftStick) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    printf("L3 button pressed\n");
                } else if (input->bRightStick) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    printf("R3 button pressed\n");
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

            if (blink_cnt % 50 == 25) {
                unique_send = 0;
                gamepad->SetVibration(0, 0);
            }
        }


        if (blink_cnt++ % 50 == 0) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        }

        if (blink_cnt % 50 == 25) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        }

        sleep_ms(16);
    }
    return 0;
}
