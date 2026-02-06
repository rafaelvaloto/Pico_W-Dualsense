cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(dualsense_test C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

pico_sdk_init()

add_compile_definitions(
        GAMEPAD_CORE_EMBEDDED=1
        GAMEPAD_CORE_EXTERNAL_SO_DEFINES="gc_config.h"
)

add_subdirectory(lib/Gamepad-Core/Source)
add_executable(dualsense_test src/main.cpp)

target_include_directories(dualsense_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(GamepadCore PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# add pico_stdlib to the GamepadCore target
target_link_libraries(GamepadCore PUBLIC pico_stdlib)

target_link_libraries(dualsense_test
        pico_stdlib
        pico_btstack_classic
        pico_cyw43_arch_none
        pico_btstack_cyw43
        hardware_gpio
        GamepadCore
)

# Configurações do Pico
pico_enable_stdio_usb(dualsense_test 1)
pico_enable_stdio_uart(dualsense_test 0)
pico_add_extra_outputs(dualsense_test)


add_custom_command(TARGET dualsense_test POST_BUILD
    COMMAND echo "Build complete: ${CMAKE_CURRENT_BINARY_DIR}/dualsense_test.elf"
    COMMAND picotool reboot -u -f || echo "Picotool: não foi possível reiniciar para BOOTSEL"
    COMMAND ${CMAKE_COMMAND} -E sleep 1.5
    COMMAND picotool load -x ${CMAKE_CURRENT_BINARY_DIR}/dualsense_test.elf -f
    COMMENT "Binário gerado. Tentando reiniciar em BOOTSEL e gravar automaticamente usando o ELF..."
)