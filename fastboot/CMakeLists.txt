cmake_minimum_required(VERSION 3.15)

project(fastboot
    VERSION 1.0.0
    LANGUAGES CXX)


set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-Wno-builtin-macro-redefined SUPPORT_CXX_WNO_BUILTIN_MACRO_REDEFINED)
if (SUPPORT_CXX_WNO_BUILTIN_MACRO_REDEFINED)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-builtin-macro-redefined")
else ()
    message("CXXCompiler can't support Wno-builtin-macro-redefined")
endif (SUPPORT_CXX_WNO_BUILTIN_MACRO_REDEFINED)

# MSVC并行编译 cmake -j xxx 对于MSVC没有作用
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/Gm->) #高版本已被废弃，但是低版本的Gm会影响并行
cmake_host_system_information(RESULT CPU_NUMBER_OF_LOGICAL_CORES QUERY NUMBER_OF_LOGICAL_CORES)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP${CPU_NUMBER_OF_LOGICAL_CORES}>)

# MSVC release generate pdb
if (MSVC)
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    SET(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
endif()

add_compile_definitions(UNICODE)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)

cmake_policy(SET CMP0087 NEW)
cmake_policy(SET CMP0079 NEW)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build Type" FORCE)
endif ()

option(ENABLE_TEST "Enable Test" OFF)
option(ENABLE_BUILD_EXAMPLE "Enable build example" OFF)

find_path(LIBUSB_INCLUDE_DIR NAMES libusb.h PATH_SUFFIXES libusb-1.0)
find_library(LIBUSB_LIBRARY NAMES libusb-1.0 libusb usb-1.0)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")
add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${LIBUSB_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../libusbwapper/src
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${LIBUSB_LIBRARY}
)

if (ENABLE_TEST)
    add_subdirectory(test)
endif ()

if (ENABLE_BUILD_EXAMPLE)
    add_subdirectory(example)
endif ()
