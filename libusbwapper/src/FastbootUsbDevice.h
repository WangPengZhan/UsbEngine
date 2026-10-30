#pragma once

#include "UsbDevice.h"

namespace libusb
{
class FastbootUsbDevice : public UsbDevice
{
public:
    FastbootUsbDevice(uint16_t vid, uint16_t pid);
    ~FastbootUsbDevice();

    uint8_t EpIn() const;
    uint8_t EpOut() const;

private:
    bool findEndpointsInfo();

private:
    uint8_t ep_in_{};
    uint8_t ep_out_{};
    uint8_t iface_{};
    uint8_t altsetting_{};
};

}  // namespace libusb
