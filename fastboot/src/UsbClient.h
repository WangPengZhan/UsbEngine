#pragma once

#include "FastbootUsbDevice.h"

namespace fastboot
{
class USBClient
{
public:
    USBClient(uint16_t vid, uint16_t pid);
    ~USBClient();

    bool connect(uint16_t vid, uint16_t pid);

    int64_t write(const uint8_t* data, int len);
    int64_t read(uint8_t* data, int len, int timeout = 3000);

    int timeoutMs() const;
    void setTimeoutMs(int ms);

    bool isValid() const;
    const std::string& lastError() const;
    size_t chunkSize() const;

private:
    std::unique_ptr<libusb::FastbootUsbDevice> device_;
    int timeoutMs_{3000};
    std::string errMsg_;
};

}  // namespace fastboot
