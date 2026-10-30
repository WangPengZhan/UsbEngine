# Build and test

[简体中文](build-and-test.zh-CN.md)

## Toolchain and dependencies

The root project requires CMake 3.15 or newer and a C++17 compiler. The source uses `std::filesystem`, which requires C++17. CI builds and runs offline tests on Windows, Linux, and macOS. The project points `CMAKE_TOOLCHAIN_FILE` at `ThirdParty/vcpkg/scripts/buildsystems/vcpkg.cmake` before the first `project()` call. The `ThirdParty/vcpkg` submodule is pinned to release tag `2026.10.16`, and `vcpkg.json` uses the same commit as its `builtin-baseline` while declaring libusb and GoogleTest.

Clone the vcpkg submodule before configuring:

```sh
git submodule update --init --recursive
```

The dependency installation is performed by the first CMake configure and can take several minutes on a clean machine.

## Configure and build

Enable tests and examples when configuring:

```sh
cmake -S . -B build -DENABLE_TEST=ON -DENABLE_BUILD_EXAMPLE=ON
```

Build on Linux or macOS:

```sh
cmake --build build --parallel
```

Build with the Visual Studio multi-configuration generator:

```powershell
cmake --build build --config Release --parallel
```

Use `-DCMAKE_BUILD_TYPE=Debug` or `Release` when configuring a single-configuration generator such as Ninja or Unix Makefiles. `--config Release` selects the configuration for Visual Studio and other multi-configuration generators.

The options `ENABLE_TEST` and `ENABLE_BUILD_EXAMPLE` default to `OFF`. Set them to `ON` to add the test and example subdirectories.

## Run all tests

After configuring with `ENABLE_TEST=ON` and building, run all registered tests with:

```sh
ctest --test-dir build --build-config Release --output-on-failure
```

For a focused test detail, CTest can run the Fastboot module by itself:

```sh
ctest --test-dir build/fastboot/test --build-config Release --output-on-failure
```

Tests that access USB hardware are skipped unless `USBENGINE_RUN_HARDWARE_TESTS=1` is set. When enabled, they may communicate with devices and transfer firmware. Review the relevant test code and connect a suitable device before enabling them. The CI workflow runs the complete CTest suite without enabling hardware tests.

## USB access

Device access depends on the host operating system and the target device. On Windows, install a libusb-compatible driver for the specific interface. On Linux, configure a narrowly scoped udev rule for the device and grant access to the intended user. Avoid running firmware operations with elevated permissions as a substitute for correct device permissions.

## Build outputs

The build creates static library targets named `libusbwapper`, `fastboot`, and `dfu`. There are currently no install or package-export targets; applications can include the project as a CMake source dependency.
