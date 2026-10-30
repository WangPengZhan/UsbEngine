cmake_minimum_required(VERSION 3.15)

set(FASTBOOT_EXAMPLE fastboot_example)

file(GLOB_RECURSE SOURCES "*.cpp" "*.h")
find_path(LIBUSB_INCLUDE_DIR NAMES libusb.h PATH_SUFFIXES libusb-1.0)

add_executable(${FASTBOOT_EXAMPLE}
    ${SOURCES}
)

target_link_libraries(${FASTBOOT_EXAMPLE}
    PRIVATE
        libusbwapper
)

target_include_directories(${FASTBOOT_EXAMPLE}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        ${LIBUSB_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../libusbwapper/src
)
