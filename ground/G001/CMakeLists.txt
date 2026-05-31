cmake_minimum_required(VERSION 3.16)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR xtensa)

set(CMAKE_CXX_COMPILER xtensa-esp32-elf-g++)
set(CMAKE_C_COMPILER xtensa-esp32-elf-gcc)
set(CMAKE_ASM_COMPILER xtensa-esp32-elf-gcc)

project(ESP32BareMetal LANGUAGES C CXX ASM)

add_compile_options(
    -ffreestanding
    -fno-exceptions
    -fno-rtti
    -nostdlib
    -fno-builtin
    -mlongcalls
    -mtext-section-literals
    -Os
)

add_executable(bare_metal boot.S main.cpp)

target_link_options(bare_metal PRIVATE
    -T ${CMAKE_CURRENT_SOURCE_DIR}/esp32.ld
    -nostdlib
    -static
)

find_program(ESPTOOL_PATH esptool.py)

if(ESPTOOL_PATH)
    # Generate the signed Espressif .bin image formatted for the internal ROM bootloader
    add_custom_command(TARGET bare_metal POST_BUILD
        COMMAND ${ESPTOOL_PATH} --chip esp32 elf2image --flash_mode dio --flash_freq 40m --flash_size 4MB -o bare_metal.bin bare_metal
        COMMENT "Converting ELF application binary to structural Espressif partition image..."
    )
else()
    message(WARNING "esptool.py utility not detected natively. Ensure your toolchain pathing is configured.")
endif()
