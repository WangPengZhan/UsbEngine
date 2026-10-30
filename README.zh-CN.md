# UsbEngine

[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17.html)
[![支持平台](https://img.shields.io/badge/%E6%94%AF%E6%8C%81%E5%B9%B3%E5%8F%B0-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)](.github/workflows/ci.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![CI](https://github.com/WangPengZhan/UsbEngine/actions/workflows/ci.yml/badge.svg)](https://github.com/WangPengZhan/UsbEngine/actions/workflows/ci.yml)

UsbEngine 是一个基于 C++17 和 [libusb](https://libusb.info/) 的 USB DFU 与 Fastboot 设备发现和通信库。仓库包含三个 CMake 构建目标：

- **libusbwapper**：管理 libusb 资源、发现 USB 设备，并读取 DFU/Fastboot 设备信息。
- **fastboot**：Fastboot 主机命令和 USB 传输。
- **dfu**：USB DFU 主机操作、DFU 文件后缀处理和 DfuSe 内存布局解析。

CMake 示例程序用于显示设备信息。库 API 也提供 Fastboot 刷写/擦除以及 DFU 固件传输功能；请仅对获准操作的设备使用对应固件。

## 功能

- 根据 DFU 或 Fastboot 接口描述符发现 USB 接口。
- 查询设备标识、传输和接口信息。
- 发送 Fastboot 命令、下载镜像、刷写/擦除分区并重启设备。
- 上传和下载标准 DFU 文件，校验 DFU 后缀并报告进度。
- 解析 DfuSe 内存布局描述符。`DfuHost` 尚未实现 DfuSe 专用镜像传输命令。

## 环境要求

- CMake 3.15 或更高版本。
- 支持 C++17 的编译器（Visual Studio 2019 或更新版本、GCC 或 Clang）。
- 支持子模块的 Git。
- 访问实体设备时，需要配置合适的 USB 驱动和设备权限。

CI 会在 Windows、Linux 和 macOS 上构建并运行离线测试。当前设备打开 API 尚不支持 Android；原因见[架构与协议范围](docs/architecture.zh-CN.md)。

根目录 CMake 项目使用 `ThirdParty/vcpkg` 子模块中的 vcpkg 工具链，固定在 release tag `2026.10.16`。依赖清单的 `builtin-baseline` 与子模块指向同一提交，并声明 libusb 和 GoogleTest。

## 获取源码并构建

递归克隆仓库和 vcpkg 子模块：

```sh
git clone --recurse-submodules https://github.com/WangPengZhan/UsbEngine.git
cd UsbEngine
```

已有仓库若尚未初始化子模块，可运行：

```sh
git submodule update --init --recursive
```

配置并构建库、测试和示例：

```sh
cmake -S . -B build -DENABLE_TEST=ON -DENABLE_BUILD_EXAMPLE=ON
cmake --build build --config Release --parallel
```

`ENABLE_TEST` 和 `ENABLE_BUILD_EXAMPLE` 默认均为 `OFF`。平台说明和测试选择见[构建与测试](docs/build-and-test.zh-CN.md)。

## 示例

示例程序会枚举设备并显示 USB 信息：

- [libusb 设备信息](libusbwapper/example/main.cpp)
- [DFU 设备信息](dfu/example/main.cpp)
- [Fastboot 设备信息](fastboot/example/main.cpp)

## 项目目录

```text
.
├── .github/                 # CI、Issue 表单、PR 模板、Dependabot
├── docs/                    # 构建说明与架构文档
├── dfu/                     # src/、test/、example/
├── fastboot/                # src/、test/、example/
├── libusbwapper/            # src/、test/、example/
├── scripts/                 # 开发辅助脚本
├── ThirdParty/vcpkg/        # vcpkg 子模块
├── CMakeLists.txt           # 根 CMake 项目
├── vcpkg.json               # 依赖清单
└── README.md
```

## 模块与 API

当前项目生成名为 `libusbwapper`、`fastboot` 和 `dfu` 的静态 CMake 目标。项目尚未提供 CMake install/export 规则；可将仓库作为源码使用，或在上层 CMake 项目中添加这些目标。

建议从以下头文件开始了解 API：

- [`libusbwapper/src/LibusbHelper.h`](libusbwapper/src/LibusbHelper.h)
- [`libusbwapper/src/DfuUsbDevice.h`](libusbwapper/src/DfuUsbDevice.h)
- [`libusbwapper/src/FastbootUsbDevice.h`](libusbwapper/src/FastbootUsbDevice.h)
- [`fastboot/src/Fastboot.h`](fastboot/src/Fastboot.h)
- [`dfu/src/DfuHost.h`](dfu/src/DfuHost.h)
- [`dfu/src/DfuFile.h`](dfu/src/DfuFile.h)

模块职责和当前协议支持范围见[架构与协议范围](docs/architecture.zh-CN.md)。

## 测试与实体设备

配置时启用 `ENABLE_TEST=ON` 后，可用这一条命令运行全部测试：

```sh
ctest --test-dir build --build-config Release --output-on-failure
```

单独运行 Fastboot 模块测试的命令示例：

```sh
ctest --test-dir build/fastboot/test --build-config Release --output-on-failure
```

USB 集成测试默认跳过；设置 `USBENGINE_RUN_HARDWARE_TESTS=1` 后才会运行。启用后测试可能与实体设备通信，部分测试会传输固件，请先阅读测试代码。CI 会运行完整 CTest 套件，但不启用硬件测试。

## 参与贡献

欢迎提交问题报告、范围明确的修复和文档改进。提交流程、构建和测试说明见[贡献指南](CONTRIBUTING.zh-CN.md)。安全问题请按[安全政策](SECURITY.zh-CN.md)私下报告。参与项目请遵守[行为准则](CODE_OF_CONDUCT.zh-CN.md)。

## 许可证

UsbEngine 根据 GNU 通用公共许可证第 3 版发布，详见 [LICENSE](LICENSE)。第三方组件遵循各自的许可证。

English: [README.md](README.md)。
