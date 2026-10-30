# Contributing

[简体中文](CONTRIBUTING.zh-CN.md)

Thanks for helping improve UsbEngine. Focused bug fixes, tests, and documentation updates are welcome.

## Before opening an issue

- Search existing issues for the same problem.
- Include the operating system, compiler, CMake version, USB device VID/PID, and exact build or runtime error when relevant.
- Do not attach firmware images, USB captures, or logs that contain secrets or private device data.

## Local build

Initialize the vcpkg submodule and configure with tests and examples enabled:

```sh
git submodule update --init --recursive
cmake -S . -B build -DENABLE_TEST=ON -DENABLE_BUILD_EXAMPLE=ON
cmake --build build --config Release --parallel
```

Run the complete test suite:

```sh
ctest --test-dir build --build-config Release --output-on-failure
```

For one focused test detail, run only the Fastboot module with CTest:

```sh
ctest --test-dir build/fastboot/test --build-config Release --output-on-failure
```

See [docs/build-and-test.md](docs/build-and-test.md) for generator-specific build settings and hardware-test notes.

## Pull requests

- Keep each change focused and explain the user-visible behavior or bug it addresses.
- Add or update offline tests for protocol and parsing changes when practical.
- Do not make tests depend on connected hardware unless they are clearly marked as integration tests.
- Update the README or documentation when build steps, API behavior, or protocol support changes.
- Include the platforms and commands you used to verify the change.
- Keep formatting consistent with nearby source files, including four-space indentation and braces around control-flow bodies.

By submitting a contribution, you agree that it is distributed under the project license shown in [LICENSE](LICENSE).
