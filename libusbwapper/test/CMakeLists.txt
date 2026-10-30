set(TestTarget libusbwapper)
set(PluginTest ${TestTarget}_test)
file(GLOB_RECURSE TEST_SOURCES LIST_DIRECTORIES false *.h *.cpp)
set(SOURCES ${TEST_SOURCES})

find_package(GTest CONFIG REQUIRED)
find_path(LIBUSB_INCLUDE_DIR NAMES libusb.h PATH_SUFFIXES libusb-1.0)

enable_testing()

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(${PluginTest} ${SOURCES})
set_target_properties(${PluginTest} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:${TestTarget}>
)
add_test(NAME ${PluginTest} COMMAND ${PluginTest})

target_include_directories(${PluginTest}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        ${LIBUSB_INCLUDE_DIR}
)

target_link_libraries(${PluginTest}
    PRIVATE
        ${TestTarget}
        GTest::gtest_main
)

add_custom_command(TARGET ${PluginTest}
    POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${TestTarget}> $<TARGET_FILE_DIR:${PluginTest}>
)