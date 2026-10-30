cmake_minimum_required(VERSION 3.15)

set(DFU_EXAMPLE dfu_example)

file(GLOB_RECURSE SOURCES "*.cpp" "*.h")
find_path(LIBUSB_INCLUDE_DIR NAMES libusb.h PATH_SUFFIXES libusb-1.0)

add_executable(${DFU_EXAMPLE}
    ${SOURCES}
)

target_link_libraries(${DFU_EXAMPLE}
    PRIVATE
        libusbwapper
)

target_include_directories(${DFU_EXAMPLE}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        ${CMAKE_CURRENT_SOURCE_DIR}/../../libusbwapper/src
        ${LIBUSB_INCLUDE_DIR}
)
