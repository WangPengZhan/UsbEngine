#include "FastbootUsbDevice.h"

#include <sstream>

namespace libusb
{

FastbootUsbDevice::FastbootUsbDevice(uint16_t vid, uint16_t pid)
    : UsbDevice(vid, pid)
{
    if (!findEndpointsInfo())
    {
        handle_.reset();
    }
}

FastbootUsbDevice::~FastbootUsbDevice()
{
    if (handle_)
    {
        libusb_release_interface(handle_.get(), iface_);
    }
}

uint8_t FastbootUsbDevice::EpIn() const
{
    return ep_in_;
}

uint8_t FastbootUsbDevice::EpOut() const
{
    return ep_out_;
}

bool FastbootUsbDevice::findEndpointsInfo()
{
    if (!handle_)
    {
        return false;
    }

    libusb_device* dev = libusb_get_device(handle_.get());
    speed_ = static_cast<libusb_speed>(libusb_get_device_speed(dev));
    libusb_config_descriptor* config = nullptr;
    if (libusb_get_active_config_descriptor(dev, &config) != 0)
    {
        return false;
    }
    LibUsbConfigDescriptorPtr configPtr(config);
    bool found = false;
    for (int i = 0; i < config->bNumInterfaces && !found; ++i)
    {
        const libusb_interface& iface = config->interface[i];
        for (int j = 0; j < iface.num_altsetting; ++j)
        {
            const libusb_interface_descriptor& alt = iface.altsetting[j];
            if (alt.bInterfaceClass != 0xff || alt.bInterfaceSubClass != 0x42 || alt.bInterfaceProtocol != 0x03)
            {
                continue;
            }

            uint8_t ep_in = 0;
            uint8_t ep_out = 0;
            uint16_t max_packet_size = 0;
            for (int k = 0; k < alt.bNumEndpoints; ++k)
            {
                const libusb_endpoint_descriptor& ep = alt.endpoint[k];
                if ((ep.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_BULK)
                {
                    if ((ep.bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN)
                    {
                        ep_in = ep.bEndpointAddress;
                    }
                    else
                    {
                        ep_out = ep.bEndpointAddress;
                    }
                    max_packet_size = std::max<uint16_t>(max_packet_size, ep.wMaxPacketSize);
                }
            }
            if (ep_in && ep_out)
            {
                found = true;
                ep_in_ = ep_in;
                ep_out_ = ep_out;
                max_packet_size_ = max_packet_size;
                iface_ = alt.bInterfaceNumber;
                altsetting_ = alt.bAlternateSetting;
                break;
            }
        }
    }

    if (!found)
    {
        errMsg_ = "No bulk in/out endpoint pair found in active config";
        return false;
    }

    int r = libusb_claim_interface(handle_.get(), iface_);
    if (r != 0)
    {
        errMsg_ = std::string("libusb_claim_interface failed: ") + libusb_error_name(r);
        return false;
    }

    if (altsetting_ != 0)
    {
        r = libusb_set_interface_alt_setting(handle_.get(), iface_, altsetting_);
        if (r != 0)
        {
            errMsg_ = std::string("libusb_set_interface_alt_setting failed: ") + libusb_error_name(r);
            libusb_release_interface(handle_.get(), iface_);
            return false;
        }
    }
    return true;
}

}  // namespace libusb
