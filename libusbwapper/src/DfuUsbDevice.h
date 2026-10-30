#pragma once
#include <string>

#include "UsbDevice.h"

namespace libusb
{

constexpr int quirkPollTimeout = (1 << 0);
constexpr int quirkForceDfu11 = (1 << 1);
constexpr int defaultPollTimeout = 50;
#pragma pack(push, 1)

struct DfuFunctionDescriptor
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bmAttributes;
    uint16_t wDetachTimeOut;
    uint16_t wTransferSize;
    uint16_t bcdDFUVersion;
};

#pragma pack(pop)

class DfuUsbDevice : public UsbDevice
{
public:
    DfuUsbDevice(uint16_t vid, uint16_t pid);
    ~DfuUsbDevice();

    const std::string& serviceName() const;
    const DfuFunctionDescriptor& descriptor() const;

    uint16_t interface() const;
    uint16_t bcdDevice() const;
    uint16_t configuration() const;
    uint16_t altsetting() const;
    uint16_t transSize() const;
    int quirks() const;

private:
    void initDFUInfo();

    void setupQuirks();
    void findDfuInfo();
    void initEndpointsInfo();

private:
    int quirks_{};
    uint16_t bcdDevice_{};
    uint16_t interface_{};
    uint16_t configuration_{};
    uint16_t altsetting_{};
    mutable std::string serviceName_{};
    DfuFunctionDescriptor descriptor_{};
    bool claimed_{};
};
}  // namespace libusb
