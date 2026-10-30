# 构建与测试

[English](build-and-test.md)

## 工具链与依赖

根项目要求 CMake 3.15 或更高版本及 C++17 编译器。源码使用 `std::filesystem`，因此必须启用 C++17。CI 在 Windows、Linux 和 macOS 上构建并运行离线测试。项目在首次调用 `project()` 之前，将 `CMAKE_TOOLCHAIN_FILE` 指向 `ThirdParty/vcpkg/scripts/buildsystems/vcpkg.cmake`。`ThirdParty/vcpkg` 子模块固定在 release tag `2026.10.16`；`vcpkg.json` 的 `builtin-baseline` 指向同一提交，并声明 libusb 和 GoogleTest 依赖。

配置前请初始化 vcpkg 子模块：

```sh
git submodule update --init --recursive
```

首次执行 CMake 配置时会安装依赖；在新环境中可能需要几分钟。

## 配置与构建

配置时启用测试和示例：

```sh
cmake -S . -B build -DENABLE_TEST=ON -DENABLE_BUILD_EXAMPLE=ON
```

在 Linux 或 macOS 上构建：

```sh
cmake --build build --parallel
```

使用 Visual Studio 多配置生成器时构建：

```powershell
cmake --build build --config Release --parallel
```

使用 Ninja 或 Unix Makefiles 等单配置生成器时，可在配置阶段设置 `-DCMAKE_BUILD_TYPE=Debug` 或 `Release`。对于 Visual Studio 等多配置生成器，`--config Release` 用于选择构建配置。

`ENABLE_TEST` 和 `ENABLE_BUILD_EXAMPLE` 默认均为 `OFF`。设为 `ON` 才会加入测试和示例子目录。

## 运行所有测试

设置 `ENABLE_TEST=ON` 并完成构建后，运行所有已注册测试：

```sh
ctest --test-dir build --build-config Release --output-on-failure
```

如果只需运行 Fastboot 模块测试：

```sh
ctest --test-dir build/fastboot/test --build-config Release --output-on-failure
```

访问 USB 硬件的测试默认跳过，只有设置 `USBENGINE_RUN_HARDWARE_TESTS=1` 后才会执行。启用后，测试可能与设备通信并传输固件。请先阅读相关测试代码，并连接合适的设备。CI 会运行完整 CTest 套件，但不启用硬件测试。

## USB 访问权限

设备访问取决于主机操作系统和目标设备。在 Windows 上，请为特定接口安装兼容 libusb 的驱动。在 Linux 上，请为设备配置范围明确的 udev 规则，并授予目标用户访问权限。不要用提升固件操作进程权限的方式替代正确的设备权限配置。

## 构建产物

构建会生成名为 `libusbwapper`、`fastboot` 和 `dfu` 的静态库目标。目前没有安装或导出包的目标；应用可以将本项目作为 CMake 源码依赖引入。
