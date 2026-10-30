# UsbEngine

[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17.html)
[![Platforms](https://img.shields.io/badge/platforms-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)](.github/workflows/ci.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![CI](https://github.com/WangPengZhan/UsbEngine/actions/workflows/ci.yml/badge.svg)](https://github.com/WangPengZhan/UsbEngine/actions/workflows/ci.yml)

UsbEngine is a C++17 library for discovering USB DFU and Fastboot devices and communicating with them through [libusb](https://libusb.info/). The repository contains three CMake targets:

- **libusbwapper**: libusb ownership, USB device discovery, and DFU/Fastboot device descriptors.
- **fastboot**: Fastboot host commands and USB transport.
- **dfu**: USB DFU host operations, DFU suffix handling, and DfuSe memory-layout parsing.

The CMake examples print device information. Fastboot flashing/erasing and DFU firmware transfers are available through the library APIs; use them only with firmware and devices you are authorized to modify.

## Features

- Discover USB interfaces by their DFU or Fastboot interface descriptors.
- Query device identifiers and transfer/interface information.
- Send Fastboot commands, download images, flash partitions, erase partitions, and reboot.
- Upload and download standard DFU files, validate DFU suffixes, and track transfer progress.
- Parse DfuSe memory-layout descriptors. DfuSe-specific image transfer commands are not implemented by `DfuHost`.

## Requirements

- CMake 3.15 or newer.
- A C++17 compiler (Visual Studio 2019 or newer, GCC, or Clang).
- Git with submodule support.
- USB device permissions and a compatible USB driver when accessing physical devices.

CI builds and runs the offline tests on Windows, Linux, and macOS. Android is not currently supported by the device-opening API; see [Architecture and protocol scope](docs/architecture.md).

The root CMake project uses the vcpkg toolchain checked out as the `ThirdParty/vcpkg` submodule, pinned to release tag `2026.10.16`. The manifest's `builtin-baseline` is pinned to the same commit and declares libusb and GoogleTest dependencies.

## Get the source and build

Clone the repository with its vcpkg submodule:

```sh
git clone --recurse-submodules https://github.com/WangPengZhan/UsbEngine.git
cd UsbEngine
```

If you already cloned without submodules, initialize them with:

```sh
git submodule update --init --recursive
```

Configure and build the libraries, tests, and examples:

```sh
cmake -S . -B build -DENABLE_TEST=ON -DENABLE_BUILD_EXAMPLE=ON
cmake --build build --config Release --parallel
```

`ENABLE_TEST` and `ENABLE_BUILD_EXAMPLE` default to `OFF`. See [Build and test](docs/build-and-test.md) for platform notes and test selection.

## Examples

The example programs enumerate a device and print its USB information:

- [libusb device information](libusbwapper/example/main.cpp)
- [DFU device information](dfu/example/main.cpp)
- [Fastboot device information](fastboot/example/main.cpp)

## Repository layout

```text
.
├── .github/                 # CI, issue forms, PR template, Dependabot
├── docs/                    # Build instructions and architecture notes
├── dfu/                     # src/, test/, example/
├── fastboot/                # src/, test/, example/
├── libusbwapper/            # src/, test/, example/
├── scripts/                 # Developer utilities
├── ThirdParty/vcpkg/        # vcpkg submodule
├── CMakeLists.txt           # Root CMake project
├── vcpkg.json               # Dependency manifest
└── README.md
```

## Modules and API notes

The project currently builds static CMake targets named `libusbwapper`, `fastboot`, and `dfu`. It does not define CMake install/export rules; consume it as a source tree or add the targets to a parent CMake project.

Start with the module headers:

- [`libusbwapper/src/LibusbHelper.h`](libusbwapper/src/LibusbHelper.h)
- [`libusbwapper/src/DfuUsbDevice.h`](libusbwapper/src/DfuUsbDevice.h)
- [`libusbwapper/src/FastbootUsbDevice.h`](libusbwapper/src/FastbootUsbDevice.h)
- [`fastboot/src/Fastboot.h`](fastboot/src/Fastboot.h)
- [`dfu/src/DfuHost.h`](dfu/src/DfuHost.h)
- [`dfu/src/DfuFile.h`](dfu/src/DfuFile.h)

The detailed module responsibilities and current protocol scope are described in [Architecture and protocol scope](docs/architecture.md).

## Tests and hardware

Build with `ENABLE_TEST=ON`, then run the entire test suite with this one command:

```sh
ctest --test-dir build --build-config Release --output-on-failure
```

For one focused test detail, CTest can run the Fastboot module by itself:

```sh
ctest --test-dir build/fastboot/test --build-config Release --output-on-failure
```

USB integration tests are skipped unless `USBENGINE_RUN_HARDWARE_TESTS=1` is set. Enabling them can communicate with physical devices; some tests transfer firmware. Read the tests before enabling them. CI runs the complete CTest suite with hardware tests left disabled.

## Contributing

Bug reports, focused fixes, and documentation improvements are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md) for the local build, test, and pull request workflow. Please follow the [Code of Conduct](CODE_OF_CONDUCT.md). Report security issues privately as described in [SECURITY.md](SECURITY.md).

## License

UsbEngine is distributed under GNU General Public License version 3. See [LICENSE](LICENSE). Third-party components retain their own licenses.

中文说明：[README.zh-CN.md](README.zh-CN.md)。
