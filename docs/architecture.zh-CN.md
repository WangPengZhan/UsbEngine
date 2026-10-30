# 架构与协议范围

[English](architecture.md)

UsbEngine 分为三个静态 CMake 目标。各模块的头文件目前位于相应模块的 `src` 目录中。

## 平台支持

项目在 CI 中于 Windows、Linux 和 macOS 上构建并运行测试。虽然 [libusb 提供 Android 后端](https://github.com/libusb/libusb/blob/master/android/README)，本项目目前仍在 [`UsbDevice.cpp`](../libusbwapper/src/UsbDevice.cpp) 中通过 `libusb_open_device_with_vid_pid()` 按 VID/PID 打开设备。Android 应用需要通过 `UsbManager` 取得权限，并将已打开的设备文件描述符传给原生代码。UsbEngine 尚未提供这部分 Android 专用集成，因此公开的设备打开 API 目前不支持 Android 设备访问。

## 仓库结构

```text
.
├── .github/                 # CI 工作流、Issue 表单、PR 模板、Dependabot
├── docs/                    # 构建与架构文档
├── dfu/
│   ├── src/                  # DFU 主机和文件处理
│   ├── test/                 # DFU 逻辑及硬件集成测试
│   └── example/              # DFU USB 设备信息示例
├── fastboot/
│   ├── src/                  # Fastboot 主机及 USB 传输
│   ├── test/                 # Fastboot 逻辑及硬件集成测试
│   └── example/              # Fastboot USB 设备信息示例
├── libusbwapper/
│   ├── src/                  # libusb 设备发现和封装
│   ├── test/                 # USB 设备集成测试
│   └── example/              # USB 设备信息示例
├── scripts/                 # 开发辅助工具
└── ThirdParty/vcpkg/        # vcpkg Git 子模块
```

三个设备信息示例分别是 [`dfu/example/main.cpp`](../dfu/example/main.cpp)、[`fastboot/example/main.cpp`](../fastboot/example/main.cpp) 和 [`libusbwapper/example/main.cpp`](../libusbwapper/example/main.cpp)。设置 `ENABLE_BUILD_EXAMPLE=ON` 即可构建。

## `libusbwapper`

此模块管理 libusb 上下文、句柄和配置描述符。`findDFUDevices()` 与 `findFastbootDevices()` 枚举当前 USB 配置，返回匹配设备的厂商 ID 和产品 ID。`DfuUsbDevice` 与 `FastbootUsbDevice` 按 VID/PID 打开设备，并提供协议客户端所需的接口信息。

使用设备前，应用应检查 `valid()` 并查看 `errorMessage()`。同一 VID/PID 可能对应多台实体设备；当前辅助 API 不支持按序列号或 USB 路径选择设备。

## `fastboot`

`FastBootHost` 通过 `AbstractTransport` 接口实现 Fastboot 命令。`UsbTransport` 将基于 libusb 的 Fastboot USB 客户端接入传输层。主机支持命令与响应处理、镜像下载、分区操作、变量查询和重启命令。

主机也可以使用自定义 `AbstractTransport` 构造，离线逻辑测试即采用这一方式。USB 集成依赖操作系统的驱动和权限设置。

## `dfu`

`DfuHost` 通过 `DfuUsbDevice` 发送标准 USB DFU 类请求。它支持 DFU 上传与下载、状态及阶段查询、中止、脱离和基于文件的传输。`DfuFile` 解析并写入标准 DFU 文件后缀，同时检查文件 CRC。

`parseMemoryLayout()` 解析 DfuSe 接口的内存布局字符串。该解析器不代表支持 DfuSe 容器传输或厂商专用命令；`DfuHost` 尚未实现这些操作。

## 错误处理与设备操作

传输和协议方法返回状态值，并在可用时提供诊断文本。调用方应检查每次操作的结果，在最终协议响应表明成功之前，不应报告传输完成。

固件传输、刷写和擦除可能改变或损坏设备。执行前请核实目标设备的 VID/PID、分区、镜像格式和恢复方式。自动化 CI 仅运行逻辑测试，不验证硬件行为。
