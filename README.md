# Pico W + DualSense (PS5) — Bluetooth Classic Firmware

A **C/C++** firmware project for the **Raspberry Pi Pico W** that connects to a **Sony DualSense** controller over **Bluetooth Classic**. It reads full controller input and can send output feedback like **lightbar**, **rumble**, and **adaptive trigger effects**.

This project integrates **BTstack** (via the Pico SDK) and links against **GamepadCore** (from `lib/Gamepad-Core`) to provide a clean, reusable abstraction for gamepad behavior.

---

## Highlights

- **Bluetooth Classic connection** to DualSense (PS5 controller)
- **Full input support**
   - Face buttons, D-pad, sticks, triggers
   - Special buttons (PS, Create, Options, Mute)
- **Output / feedback**
   - Lightbar (RGB)
   - Rumble
   - Adaptive trigger effects (L2/R2)
- **Extended report handling**
   - Automatically handles extended report layouts (e.g., `0x31`) to keep parsing consistent
- **Persistent pairing**
   - Saves the controller MAC address in Pico W flash for easier reconnection

---

## Built With

- **Raspberry Pi Pico SDK**
- **BTstack** (Pico SDK integration for CYW43)
- **GamepadCore** (linked as `GamepadCore` from `lib/Gamepad-Core/Source`)
- CMake + ARM GCC toolchain

---

## Repository Layout (key files)

- `src/main.cpp`  
  Entry point, hardware init, and main loop.

- `src/pico_w_platform.h`  
  Bluetooth/L2CAP handling and the glue layer that bridges BTstack packets to GamepadCore.

- `src/pico_w_flash_ptr.h`  
  Flash helpers used for storing persistent data (e.g., MAC address).

- `lib/Gamepad-Core/`  
  **GamepadCore library** providing device abstractions and DualSense logic shared across platforms.

---

## Default Test Controls

| Input | Effect |
| --- | --- |
| Cross (X) | Heavy rumble + red lightbar |
| Circle (O) | Soft rumble + blue lightbar |
| Square (□) | Trigger effect preset (R2) |
| Triangle (△) | Stop all effects |
| L1 | Trigger effect preset (L2) |
| R1 | Trigger effect preset (R2) |
| D-pad Up | Trigger effect preset (rigid feedback) |
| D-pad Down | Trigger effect preset (tension/bow) |
| D-pad Left | Trigger effect preset (semi/weapon) |
| D-pad Right | Trigger effect preset (buzz/automatic) |

> The exact presets can be adjusted in code depending on what you want to test.

---

## Requirements

- Linux/macOS/Windows host (Linux recommended)
- **ARM GCC** toolchain (e.g., `arm-none-eabi-gcc`)
- **CMake**
- **Pico SDK** available locally
- A **Raspberry Pi Pico W**
- A **Sony DualSense** controller

Optional (useful):
- Serial monitor (`minicom`, `screen`, etc.)
- `picotool` for device info/debugging

---

## Build Instructions

1. Clone the repository (and submodules if you use them):
   ```bash
   git submodule update --init --recursive
   ```

2. Set `PICO_SDK_PATH`:
   ```bash
   export PICO_SDK_PATH=/path/to/pico-sdk
   ```

3. Configure and build:
   ```bash
   mkdir -p build
   cd build
   cmake ..
   make -j
   ```

4. Output:
   - `dualsense_test.uf2`

---

## Flashing the UF2 (Pico W)

1. Hold **BOOTSEL** and plug the Pico W into USB.
2. The Pico W should mount as a drive (often `RPI-RP2`).
3. Copy the UF2:
   ```bash
   cp build/dualsense_test.uf2 /media/$USER/RPI-RP2/
   ```

The board will reboot automatically after the copy completes.

---

## Pairing the DualSense

1. Power the Pico W with the flashed firmware.
2. Put the DualSense in pairing mode:
- Hold **PS + Create** for ~3 seconds until the light flashes rapidly.
3. The Pico W should discover and connect.
4. On success, input events should be processed and your test outputs should work.

> Tip: enable/keep USB serial logs to see connection state, discovery results, and L2CAP status.

---

## Troubleshooting

- **Controller not discovered**
   - Confirm the DualSense is in fast-blinking pairing mode.
   - Keep it close to the Pico W.
   - Restart Pico W and try again.

- **Connects but no input**
   - Confirm the interrupt/control channels are established (if you log that state).
   - Verify the report format your parsing expects (DualSense can vary depending on mode/report type).

- **Reconnect issues**
   - If a MAC is stored and reconnection fails, the controller may be bonded to another host.
   - Consider adding a “clear stored MAC” option (serial command) if you iterate often.

---

## Roadmap (Ideas)

- [ ] Serial CLI to change test profiles (LED/rumble/trigger presets)
- [ ] Multiple stored devices (MAC list)
- [ ] Battery/charging telemetry reporting
- [ ] Cleaner state machine logs (scan → pair → connect → ready)

---

## License

MIT — see [LICENSE](LICENSE).

---

## Credits

- Raspberry Pi Pico SDK
- BTstack
- GamepadCore (library linked from `lib/Gamepad-Core`)