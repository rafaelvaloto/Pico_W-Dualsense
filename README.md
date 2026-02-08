<div align="center">

# 🎮 Pico W + DualSense (PS5)
### Bluetooth Classic Firmware

**A Portable Proof of Concept powered by [Gamepad-Core](https://github.com/rafaelvaloto/Gamepad-Core)**

[![Raspberry Pi Pico W](https://img.shields.io/badge/Pico_W-Compatible-C51A4A?style=for-the-badge&logo=raspberrypi&logoColor=white)](https://www.raspberrypi.com/products/raspberry-pi-pico/)
[![DualSense](https://img.shields.io/badge/DualSense-Supported-003791?style=for-the-badge&logo=playstation&logoColor=white)](https://www.playstation.com/accessories/dualsense-wireless-controller/)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg?style=for-the-badge)](LICENSE)

**C++ • BTstack • Gamepad-Core**

</div>

---

## 🚀 Project Overview

This project is a **C/C++ firmware** for the **Raspberry Pi Pico W** that establishes a full-featured **Bluetooth Classic connection** with the **Sony DualSense (PS5)** controller. It demonstrates the extreme portability of the **[Gamepad-Core](https://github.com/rafaelvaloto/Gamepad-Core)** library by running the same cross-platform API that powers high-end game engines (Unreal Engine, Godot, O3DE) on a resource-constrained microcontroller.

### 🎯 Why This Project Exists

This firmware was created to **stress-test the portability** of the Gamepad-Core library. It proves that the same library running on powerful desktop machines and game engines can also run smoothly on a **$6 microcontroller with limited resources**.

**Key Achievement**: A unified codebase for DualSense support across:
- 🖥️ Desktop platforms (Windows, Linux, macOS)
- 🎮 Game engines (Unreal Engine, Godot, O3DE)
- 🔧 Microcontrollers (Pico W, ESP32, and more)

---

### 🎥 [Click and watch the example video on YouTube.](https://www.youtube.com/watch?v=GgKDtwfS6v4)


## ✨ Features

### ✅ Currently Implemented

- **Stable Bluetooth Classic Connection**: Reliable pairing and connectivity with DualSense controllers
- **Persistent Pairing**: Stores controller MAC address in Pico W's flash memory for automatic reconnection
- **Full Input Reading**:
  - Extended reports (`0x31`) unlocking all advanced features:
    - All buttons and analog sticks
    - Gyroscope and accelerometer data
    - Touchpad input
    - Battery status
- **Complete Output Features**:
  - 💡 **LED Control**: Lightbar colors and player LED indicators
  - 📳 **Rumble/Haptics**: Vibration motor control (soft and heavy)
  - 🎚️ **Adaptive Triggers**: Full trigger effect system
    - Resistance modes (Feedback, Weapon, Vibration)
    - Dynamic tension effects (Bow, Gallop, Machine)
    - Weapon simulation (GameCube, Semi-Automatic, Automatic)
- **Plug-and-Play Integration**: Powered by Gamepad-Core's abstraction layer

### 🎮 Interactive Test Demo

The firmware includes a comprehensive test suite demonstrating all DualSense features:

```
=======================================================
           DUALSENSE INTEGRATION TEST                  
=======================================================

 [ FACE BUTTONS ]
   (X) Cross    : Heavy Rumble + RED Light
   (O) Circle   : Soft Rumble  + YELLOW Light
   [ ] Square   : Trigger Effect: GAMECUBE (R2)
   /_\ Triangle : Stop All

-------------------------------------------------------

 [ D-PADS & SHOULDERS ]
   [L1]    : Trigger Effect: Gallop (L2)
   [R1]    : Trigger Effect: Machine (R2)
   [UP]    : Trigger Effect: Feedback (Rigid)
   [DOWN]  : Trigger Effect: Bow (Tension)
   [LEFT]  : Trigger Effect: Weapon (Semi)
   [RIGHT] : Trigger Effect: Automatic Gun (Buzz)

=======================================================
```

---

## 🔧 Hardware Requirements

- **Raspberry Pi Pico W** (with wireless capabilities)
- **Sony DualSense Controller** (PS5)
- USB cable for programming the Pico W

---

## 📚 Software Requirements

- **Pico SDK** (with BTstack integrated)
- **Gamepad-Core library** (minimalist embedded version)
- CMake 3.20+
- C++20 compatible compiler

---

## 🛠️ Setup Instructions

### 1. Clone This Repository

```bash
git clone https://github.com/rafaelvaloto/Pico_W-Dualsense.git
cd Pico_W-Dualsense
```

### 2. Download Gamepad-Core (Minimalist Version)

Clone the minimalist version of **Gamepad-Core** to a directory **outside** your Pico project:

```bash
# Clone to a separate dependencies folder (not inside the Pico project)
cd ..
git clone --depth 1 https://github.com/rafaelvaloto/Gamepad-Core.git
```

Your directory structure should look like this:
```
parent-folder/
├── Pico_W-Dualsense/        # Your Pico project
└── Gamepad-Core/            # Gamepad-Core library (separate)
```

### 3. Export for Embedded Systems

Navigate to the Gamepad-Core folder and run the export script, pointing it to your Pico project's `lib` folder:

```bash
cd Gamepad-Core
./export_micro.sh ../Pico_W-Dualsense/lib
```

This script will:
- Copy necessary headers and source files to your Pico project
- Organize files for embedded compilation
- Remove unnecessary dependencies for microcontroller use

### 4. Configure SDK Integration (`gc_config.h`)

Create a `gc_config.h` file at the root of your Pico project to map Gamepad-Core's internal functions to Pico SDK functions:

```cpp name=gc_config.h
//
// Gamepad-Core Configuration for Pico SDK
//
#pragma once

#if defined(GAMEPAD_CORE_EMBEDDED)
    #include "pico/stdlib.h"

    #ifndef gc_sleep_ms
        #define gc_sleep_ms ::sleep_ms
    #endif
#endif
```

### 5. Build the Firmware

```bash
cd Pico_W-Dualsense
mkdir build && cd build
cmake ..
make
```

### 6. Flash to Pico W

1. Hold the **BOOTSEL** button on your Pico W
2. Connect it to your computer via USB
3. Copy the generated `*.uf2` file to the Pico's mass storage device
4. The Pico will automatically reboot and start running the firmware

---

## 📖 How It Works

### Bluetooth Connection Flow

1. **Pairing Mode**: On first run, the Pico W enters pairing mode
2. **Controller Discovery**: Put your DualSense in pairing mode (hold PS + Share buttons)
3. **Connection**: The controller connects and the pairing key is stored in flash
4. **Auto-Reconnect**: On subsequent power-ups, the Pico automatically reconnects to the paired controller

### Report ID 0x31 - Advanced Features

The firmware automatically configures the DualSense to use **Report ID 0x31**, which unlocks:
- Full sensor data (gyro, accelerometer)
- Touchpad input
- Battery status
- Support for all output features (haptics, LEDs, adaptive triggers)

### Integration with BTstack

The `packet_handler` in `src/pico_w_platform.h` processes L2CAP packets from the controller:
- **`0x31` packets**: Read with offset 2 (extended input)

Data is copied directly to the `Context->Buffer` of Gamepad-Core, which handles all protocol decoding and abstraction.

### Output Features via Bluetooth

The firmware uses Gamepad-Core's output buffer system to send commands back to the DualSense:
- **LED commands**: Set lightbar RGB colors and player indicators
- **Rumble commands**: Control left/right motors independently
- **Trigger effects**: Configure adaptive triggers with various resistance patterns

All output features use the same `0x31` report format for bidirectional communication.

### Main Loop Architecture

```cpp
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
```

Library processing (PlugAndPlay and Updates) occurs in the main loop, ensuring operations requiring mutexes or delays don't block the Bluetooth interrupt handler.

---

## 🎮 About Gamepad-Core

**[Gamepad-Core](https://github.com/rafaelvaloto/Gamepad-Core)** is a revolutionary DualSense & DualShock library designed for true cross-platform portability:

### Core Philosophy
- ✅ **Pure C++20**: No external dependencies
- ✅ **Engine Agnostic**: Works with any game engine or framework
- ✅ **Platform Independent**: One codebase for all platforms
- ✅ **Embedded Ready**: Runs on microcontrollers with minimal resources

### Proven Compatibility
- **Game Engines**: Unreal Engine 5, Godot 4, O3DE
- **Operating Systems**: Windows, Linux, macOS, PlayStation
- **Microcontrollers**: Raspberry Pi Pico W, ESP32, and more

### Architecture Highlights
- **Clean Abstraction**: Simple API for complex controller features
- **Buffer Management**: Efficient input/output buffer handling
- **Event System**: Callback-based controller events
- **Minimal Footprint**: Optimized for embedded systems

**[→ Visit Gamepad-Core Repository](https://github.com/rafaelvaloto/Gamepad-Core)**

---

## 🔬 Technical Details

### Supported DualSense Features

| Feature | Status |
|---------|--------|
| Buttons (×17) | ✅ Fully Working |
| Analog Sticks (L/R) | ✅ Fully Working |
| Triggers (L2/R2) | ✅ Fully Working |
| D-Pad | ✅ Fully Working |
| Gyroscope | ✅ Fully Working |
| Accelerometer | ✅ Fully Working |
| Touchpad | ✅ Fully Working |
| Battery Status | ✅ Fully Working |
| Lightbar RGB | ✅ Fully Working |
| Player LED | ✅ Fully Working |
| Rumble Motors | ✅ Fully Working |
| Adaptive Triggers | ✅ Fully Working |
| Audio | ❌ Not planned |

> **Legend**: ✅ = Fully working | ❌ = Not implemented

---

## 🤝 Contributing

Contributions are welcome! Whether it's:
- Optimizing performance
- Adding support for other controllers (DualShock 4, Nintendo Pro, etc.)
- Improving documentation
- Adding new trigger effects
- Implementing audio support

Feel free to open issues or submit pull requests.

---

## 📜 License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

---

## 🙏 Acknowledgments

- **[BTstack](https://github.com/bluekitchen/btstack)**: Bluetooth stack for embedded systems
- **[Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)**: Official SDK for RP2040
- **[Gamepad-Core](https://github.com/rafaelvaloto/Gamepad-Core)**: The cross-platform magic behind this project

---

## 📬 Contact & Support

- **Author**: Rafael Valoto
- **Repository Issues**: [Report a Bug](https://github.com/rafaelvaloto/Pico_W-Dualsense/issues)
- **Gamepad-Core Issues**: [Library Support](https://github.com/rafaelvaloto/Gamepad-Core/issues)

---
