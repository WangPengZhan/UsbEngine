cmake_minimum_required(VERSION 3.15)

set(LIBUSBWAPPER_EXAMPLE libusbwapper_example)

file(GLOB_RECURSE SOURCES "*.cpp" "*.h")
find_path(LIBUSB_INCLUDE_DIR NAMES libusb.h PATH_SUFFIXES libusb-1.0)

add_executable(${LIBUSBWAPPER_EXAMPLE}
    ${SOURCES}
)

target_link_libraries(${LIBUSBWAPPER_EXAMPLE}
    PRIVATE
        libusbwapper
)

target_include_directories(${LIBUSBWAPPER_EXAMPLE}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        ${LIBUSB_INCLUDE_DIR}
)
