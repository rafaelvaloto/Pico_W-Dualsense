# Pico W + DualSense (PS5) — Bluetooth Classic Firmware

This project is a **C/C++** firmware for the **Raspberry Pi Pico W** that connects to a **Sony DualSense** controller via **Bluetooth Classic**. It allows for full reading of controller inputs and sending feedback such as **lightbar**, **rumble**, and **adaptive trigger effects**.

The project uses **BTstack** (integrated into the Pico SDK) and the **GamepadCore** library for controller logic abstraction.

---

## Implementation Status

Currently, the project supports:
- Stable **Bluetooth Classic connection** with DualSense.
- **Input Buffer Reading**:
    - Support for basic reports (`0x01`).
    - Support for extended reports (`0x31`), ensuring all buttons, analogs, and sensors are read correctly.
- **Output Report Sending**:
    - LED control (Lightbar and Player LED).
    - Rumble (Vibration motors).
    - Adaptive Triggers (Presets for resistance, trigger, machine gun, etc.).
- **Persistence**: Storage of the controller's MAC address in the Pico W's Flash for automatic reconnection.

> **Note**: Audio support via Bluetooth Classic is not yet implemented in this version.

---

## How to Use the LIB (GamepadCore)

### 1. Download GamepadCore (Minimalist Version)
To use this project, you must download the minimalist version of `GamepadCore` (without unnecessary submodules) into a folder of your choice where you manage dependencies.

```bash
# Example of how to download (adjust to the actual minimalist lib link if necessary)
git clone --depth 1 https://github.com/rafaelvaloto/GamepadCore.git lib/Gamepad-Core
```

### 2. Exporting for Microcontrollers
After downloading, navigate to the library folder and execute the export script to generate the structure compatible with embedded systems:

```bash
cd lib/Gamepad-Core
./export_micro.sh
```
This will organize the necessary files to be included in the Pico W project.

### 3. SDK Configuration (gc_config.h)
For the library to work correctly in the Pico SDK (RP2040) environment, it is necessary to create a file called `gc_config.h` at the root of your project. This file maps internal lib functions (such as sleep) to the Raspberry Pi SDK functions.

Example content for `gc_config.h`:

```cpp
//
// Created by rafaelvaloto on 03/02/2026.
//
#pragma once

#if defined(GAMEPAD_CORE_EMBEDDED)
    #include "pico/stdlib.h"

    #ifndef gc_sleep_ms
        #define gc_sleep_ms ::sleep_ms
    #endif
#endif
```

---

## Implementation Details

### Integration with BTstack
Integration is done in the `src/pico_w_platform.h` file. The `packet_handler` processes L2CAP packets coming from the controller:
- `0x01` packets are read with offset 1.
- `0x31` packets are read with offset 2.

Data is copied directly to the `Context->Buffer` of `GamepadCore`, which then decodes the buttons and axes for use in `main.cpp`.

### Main Loop
Library processing (PlugAndPlay and Updates) occurs in the main loop of `main.cpp`, ensuring that operations requiring mutexes or waits do not hang the Bluetooth interrupt handler.

---