# Architecture and protocol scope

[简体中文](architecture.zh-CN.md)

UsbEngine is split into three static CMake targets. The source headers currently live under each module's `src` directory.

## Platform support

The project is built and tested in CI on Windows, Linux, and macOS. Although [libusb has an Android backend](https://github.com/libusb/libusb/blob/master/android/README), this project currently opens devices by VID/PID with `libusb_open_device_with_vid_pid()` in [`UsbDevice.cpp`](../libusbwapper/src/UsbDevice.cpp). Android applications need to obtain permission through `UsbManager` and pass the opened device file descriptor to native code. UsbEngine does not yet provide that Android-specific integration, so Android device access is not currently supported by the public device-opening API.

## Repository layout

```text
.
├── .github/                 # CI workflow, issue forms, PR template, Dependabot
├── docs/                    # Build and architecture documentation
├── dfu/
│   ├── src/                  # DFU host and file handling
│   ├── test/                # DFU logic and hardware integration tests
│   └── example/             # DFU USB device inspection example
├── fastboot/
│   ├── src/                  # Fastboot host and USB transport
│   ├── test/                # Fastboot logic and hardware integration tests
│   └── example/             # Fastboot USB device inspection example
├── libusbwapper/
│   ├── src/                  # libusb device discovery and wrappers
│   ├── test/                # USB device integration test
│   └── example/             # USB device inspection example
├── scripts/                 # Developer utilities
└── ThirdParty/vcpkg/        # vcpkg Git submodule
```

The three device inspection examples are [`dfu/example/main.cpp`](../dfu/example/main.cpp), [`fastboot/example/main.cpp`](../fastboot/example/main.cpp), and [`libusbwapper/example/main.cpp`](../libusbwapper/example/main.cpp). Build them by setting `ENABLE_BUILD_EXAMPLE=ON`.

## `libusbwapper`

This module owns libusb contexts, handles, and configuration descriptors. `findDFUDevices()` and `findFastbootDevices()` enumerate active USB configurations and return matching vendor/product identifiers. `DfuUsbDevice` and `FastbootUsbDevice` open a device by VID/PID and expose interface details needed by their protocol clients.

Applications should check `valid()` and inspect `errorMessage()` before using a device. A VID/PID pair may match more than one physical device; the current helper API does not select by serial number or USB path.

## `fastboot`

`FastBootHost` implements Fastboot commands over the `AbstractTransport` interface. `UsbTransport` adapts the libusb-backed Fastboot USB client. The host supports command/response handling, image downloads, partition operations, variable queries, and reboot commands.

The host can also be constructed with a custom `AbstractTransport`, which is used by the offline logic tests. USB integration depends on the operating system's driver and permissions.

## `dfu`

`DfuHost` sends standard USB DFU class requests through `DfuUsbDevice`. It supports DFU upload/download, status and state requests, abort, detach, and file-based transfer. `DfuFile` parses and writes the standard DFU suffix and checks the file CRC.

`parseMemoryLayout()` parses DfuSe interface memory-layout strings. This parser does not imply support for DfuSe container transfer or its vendor-specific commands; those operations are not implemented by `DfuHost`.

## Error handling and device operations

Transport and protocol methods return status values and expose diagnostic text where available. Callers should check each operation's result and avoid reporting a transfer as complete until the final protocol response indicates success.

Firmware transfer, flashing, and erasing can change or damage a device. Verify the VID/PID, partition, image format, and recovery method for the target before invoking those operations. The automated CI workflow runs only logic tests and does not validate hardware behavior.
