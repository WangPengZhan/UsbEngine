# 贡献指南

[English](CONTRIBUTING.md)

感谢帮助改进 UsbEngine。欢迎提交范围明确的缺陷修复、测试和文档更新。

## 提交 Issue 前

- 先搜索已有 Issue，确认是否有人报告过相同问题。
- 如有必要，请提供操作系统、编译器、CMake 版本、USB 设备的 VID/PID，以及准确的构建或运行错误。
- 不要附上包含密钥或设备私有数据的固件镜像、USB 抓包或日志。

## 本地构建

初始化 vcpkg 子模块，并启用测试和示例：

```sh
git submodule update --init --recursive
cmake -S . -B build -DENABLE_TEST=ON -DENABLE_BUILD_EXAMPLE=ON
cmake --build build --config Release --parallel
```

运行完整测试套件：

```sh
ctest --test-dir build --build-config Release --output-on-failure
```

如果只需运行 Fastboot 模块测试：

```sh
ctest --test-dir build/fastboot/test --build-config Release --output-on-failure
```

生成器相关的构建设置和硬件测试说明见[构建与测试](docs/build-and-test.zh-CN.md)。

## Pull Request

- 每次修改应聚焦一个问题，并说明它解决的用户可见行为或缺陷。
- 修改协议或解析逻辑时，尽可能添加或更新离线测试。
- 除非明确标记为集成测试，否则不要让测试依赖已连接的硬件。
- 构建步骤、API 行为或协议支持范围发生变化时，更新 README 或相关文档。
- 写明验证修改时使用的平台和命令。
- 保持与相邻源文件一致的格式，包括四空格缩进和控制流语句的花括号。

提交贡献即表示你同意按项目的 [LICENSE](LICENSE) 发布该贡献。
