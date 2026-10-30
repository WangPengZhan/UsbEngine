#include "DfuUsbDevice.h"

#include <cstring>

namespace libusb
{

DfuUsbDevice::DfuUsbDevice(uint16_t vid, uint16_t pid)
    : UsbDevice(vid, pid)
{
    initDFUInfo();
}

DfuUsbDevice::~DfuUsbDevice()
{
    if (handle_ && claimed_)
    {
        libusb_release_interface(handle_.get(), interface_);
    }
}

const std::string& DfuUsbDevice::serviceName() const
{
    return serviceName_;
}

const DfuFunctionDescriptor& DfuUsbDevice::descriptor() const
{
    return descriptor_;
}

uint16_t DfuUsbDevice::bcdDevice() const
{
    return bcdDevice_;
}

uint16_t DfuUsbDevice::configuration() const
{
    return configuration_;
}

uint16_t DfuUsbDevice::altsetting() const
{
    return altsetting_;
}

uint16_t DfuUsbDevice::transSize() const
{
    return descriptor_.wTransferSize;
}

uint16_t DfuUsbDevice::interface() const
{
    return interface_;
}

int DfuUsbDevice::quirks() const
{
    return quirks_;
}

void DfuUsbDevice::initDFUInfo()
{
    if (!handle_)
    {
        return;
    }

    findDfuInfo();
    if (!handle_)
    {
        return;
    }

    setupQuirks();
    initEndpointsInfo();
}

void DfuUsbDevice::setupQuirks()
{
    if (!handle_)
    {
        return;
    }
    constexpr uint16_t vidOpenmoko = 0x1d50;  // Openmoko Freerunner / GTA02
    constexpr uint16_t vidFIC = 0x1457;       // Openmoko Freerunner / GTA02
    constexpr uint16_t VidVOTI = 0x16c0;      // OpenPCD Reader
    constexpr uint16_t vidLeaflabs = 0x1eaf;  // Maple
    constexpr uint16_t pidMaple3 = 0x0003;    // rev 3 and 5
    constexpr int quirkPollTimeout = (1 << 0);
    constexpr int quirkForceDfu11 = (1 << 1);
    if (vid() == vidOpenmoko || vid() == vidFIC || vid() == VidVOTI)
    {
        quirks_ |= quirkPollTimeout;
    }
    if (vid() == vidLeaflabs && pid() == pidMaple3 && bcdDevice_ == 0x0200)
    {
        quirks_ |= quirkForceDfu11;
    }
}

void DfuUsbDevice::findDfuInfo()
{
    descriptor_ = {};
    libusb_device* dev = libusb_get_device(handle_.get());
    libusb_device_descriptor desc{};
    if (libusb_get_device_descriptor(dev, &desc) != 0)
    {
        errMsg_ = "Failed to read device descriptor";
        handle_.reset();
        return;
    }
    libusb_config_descriptor* config = nullptr;
    if (libusb_get_active_config_descriptor(dev, &config) != 0 || !config)
    {
        errMsg_ = "Failed to read active configuration";
        handle_.reset();
        return;
    }

    LibUsbConfigDescriptorPtr configPtr(config);
    for (int interfaceindex = 0; interfaceindex < config->bNumInterfaces; ++interfaceindex)
    {
        const auto& uif = config->interface[interfaceindex];
        for (int alt_idx = 0; alt_idx < uif.num_altsetting; ++alt_idx)
        {
            const auto& intf = uif.altsetting[alt_idx];
            if (intf.bInterfaceClass == 0xfe && intf.bInterfaceSubClass == 1)
            {
                bcdDevice_ = desc.bcdDevice;
                interface_ = intf.bInterfaceNumber;
                configuration_ = config->bConfigurationValue;
                altsetting_ = intf.bAlternateSetting;

                if (intf.iInterface)
                {
                    unsigned char nameBuf[255] = {};
                    int length = libusb_get_string_descriptor_ascii(handle(), intf.iInterface, nameBuf, sizeof(nameBuf));
                    if (length > 0)
                    {
                        serviceName_.assign(reinterpret_cast<const char*>(nameBuf), length);
                    }
                }

                const unsigned char* extra = intf.extra;
                int extra_len = intf.extra_length;
                while (extra && extra_len >= 2)
                {
                    uint8_t length = extra[0];
                    if (length < 2 || length > extra_len)
                    {
                        break;
                    }
                    if (extra[1] == 0x21 && length >= sizeof(descriptor_))
                    {
                        memcpy(&descriptor_, extra, sizeof(descriptor_));
                        break;
                    }
                    extra_len -= length;
                    extra += length;
                }
                return;
            }
        }
    }

    errMsg_ = "No DFU interface found in active configuration";
    handle_.reset();
}

void DfuUsbDevice::initEndpointsInfo()
{
    int r = libusb_claim_interface(handle_.get(), interface());
    if (r != 0)
    {
        errMsg_ = std::string("libusb_claim_interface failed: ") + libusb_error_name(r);
        handle_.reset();
        return;
    }
    claimed_ = true;

    r = libusb_set_interface_alt_setting(handle_.get(), interface(), altsetting());
    if (r != 0)
    {
        errMsg_ = std::string("libusb_set_interface_alt_setting failed: ") + libusb_error_name(r);
        libusb_release_interface(handle_.get(), interface());
        claimed_ = false;
        handle_.reset();
        return;
    }
}

}  // namespace libusb
