#pragma once
#include <string>

#include "LibusbHelper.h"

namespace libusb
{

class UsbDevice
{
public:
    UsbDevice(uint16_t vid, uint16_t pid);
    ~UsbDevice();

    bool valid() const;

    uint16_t vid() const;
    uint16_t pid() const;

    const std::string& name() const;
    size_t chunkSize() const;
    std::string speedInfo() const;
    uint16_t ep0MaxPacketSize() const;

    libusb_device_handle* handle() const;
    const std::string& errorMessage() const;

protected:
    std::string name_{};
    uint16_t vid_;
    uint16_t pid_;
    libusb_speed speed_{};
    LibUsbContextPtr context_;
    LibUsbDeviceHandlePtr handle_;
    uint16_t max_packet_size_{};
    std::string errMsg_;
};

}  // namespace  libusb
