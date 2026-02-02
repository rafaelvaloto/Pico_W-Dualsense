cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(dualsense_test C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

pico_sdk_init()

add_executable(dualsense_test
        src/main.cpp
)

# Includes
target_include_directories(dualsense_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Libraries - simple configuration for Bluetooth Classic
add_subdirectory(lib/Gamepad-Core/Source)

target_link_libraries(dualsense_test
        pico_stdlib
        pico_stdlib
        pico_btstack_classic
        pico_cyw43_arch_none       # CYW43 without WiFi/lwIP
        pico_btstack_cyw43         # BTstack for CYW43
        hardware_gpio
        GamepadCore
)

# USB serial for debug
pico_enable_stdio_usb(dualsense_test 1)
pico_enable_stdio_uart(dualsense_test 0)

pico_add_extra_outputs(dualsense_test)

# Custom commands with picotool
# Automatic upload and reboot after compilation
add_custom_command(TARGET dualsense_test POST_BUILD
    COMMAND echo "Build complete: ${CMAKE_CURRENT_BINARY_DIR}/dualsense_test.uf2"
    COMMENT "Binary generated successfully!"
)


