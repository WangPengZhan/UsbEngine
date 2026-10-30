#include "LibusbHelper.h"

namespace libusb
{
LibUsbContextPtr makeUsbContext()
{
    libusb_context* ctx = nullptr;
    if (libusb_init(&ctx) != 0)
    {
        return {};
    }
    return LibUsbContextPtr(ctx);
}

std::vector<std::pair<uint32_t, uint32_t>> findDFUDevices()
{
    std::vector<std::pair<uint32_t, uint32_t>> res;
    LibUsbContextPtr ctx = makeUsbContext();
    if (!ctx)
    {
        return res;
    }

    libusb_device** list = nullptr;
    ssize_t count = libusb_get_device_list(ctx.get(), &list);
    if (count < 0)
    {
        return res;
    }

    for (ssize_t i = 0; i < count; ++i)
    {
        libusb_device* dev = list[i];
        libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(dev, &desc) != 0)
        {
            continue;
        }

        libusb_config_descriptor* config = nullptr;
        if (libusb_get_active_config_descriptor(dev, &config) != 0 || !config)
        {
            continue;
        }

        LibUsbConfigDescriptorPtr configPtr(config);
        bool isDFU = false;
        for (int interfaceIndex = 0; interfaceIndex < config->bNumInterfaces; ++interfaceIndex)
        {
            const libusb_interface& interface = config->interface[interfaceIndex];
            for (int alternateIndex = 0; alternateIndex < interface.num_altsetting; ++alternateIndex)
            {
                const libusb_interface_descriptor& alternate = interface.altsetting[alternateIndex];
                if (alternate.bInterfaceClass == 0xFE && alternate.bInterfaceSubClass == 0x01)
                {
                    isDFU = true;
                    break;
                }
            }

            if (isDFU)
            {
                break;
            }
        }

        if (isDFU)
        {
            res.emplace_back(desc.idVendor, desc.idProduct);
        }
    }
    libusb_free_device_list(list, 1);
    return res;
}

std::vector<std::pair<uint32_t, uint32_t>> findFastbootDevices()
{
    std::vector<std::pair<uint32_t, uint32_t>> res;
    LibUsbContextPtr ctx = makeUsbContext();
    if (!ctx)
    {
        return res;
    }

    libusb_device** list = nullptr;
    ssize_t count = libusb_get_device_list(ctx.get(), &list);
    if (count < 0)
    {
        return res;
    }

    for (ssize_t i = 0; i < count; ++i)
    {
        libusb_device* dev = list[i];
        libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(dev, &desc) != 0)
        {
            continue;
        }

        libusb_config_descriptor* config = nullptr;
        if (libusb_get_active_config_descriptor(dev, &config) != 0 || !config)
        {
            continue;
        }
        LibUsbConfigDescriptorPtr configPtr(config);
        bool found = false;
        for (int interfaceIndex = 0; interfaceIndex < config->bNumInterfaces && !found; ++interfaceIndex)
        {
            const libusb_interface& interface = config->interface[interfaceIndex];
            for (int alternateIndex = 0; alternateIndex < interface.num_altsetting; ++alternateIndex)
            {
                const libusb_interface_descriptor& alternate = interface.altsetting[alternateIndex];
                if (alternate.bInterfaceClass != 0xff || alternate.bInterfaceSubClass != 0x42 || alternate.bInterfaceProtocol != 0x03)
                {
                    continue;
                }

                bool hasInputEndpoint = false;
                bool hasOutputEndpoint = false;
                for (int endpointIndex = 0; endpointIndex < alternate.bNumEndpoints; ++endpointIndex)
                {
                    const libusb_endpoint_descriptor& endpoint = alternate.endpoint[endpointIndex];
                    if ((endpoint.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) != LIBUSB_TRANSFER_TYPE_BULK)
                    {
                        continue;
                    }

                    if ((endpoint.bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN)
                    {
                        hasInputEndpoint = true;
                    }
                    else
                    {
                        hasOutputEndpoint = true;
                    }
                }
                if (hasInputEndpoint && hasOutputEndpoint)
                {
                    found = true;
                    break;
                }
            }
        }
        if (!found)
        {
            continue;
        }
        res.emplace_back(desc.idVendor, desc.idProduct);
    }

    libusb_free_device_list(list, 1);
    return res;
}

std::string speed_str(libusb_speed speed)
{
    switch (speed)
    {
    case LIBUSB_SPEED_LOW:
        return "1.5 Mbps (USB 1.1 Low)";
    case LIBUSB_SPEED_FULL:
        return "12 Mbps (USB 1.1 Full)";
    case LIBUSB_SPEED_HIGH:
        return "480 Mbps (USB 2.0)";
    case LIBUSB_SPEED_SUPER:
        return "5 Gbps (USB 3.0)";
    case LIBUSB_SPEED_SUPER_PLUS:
        return "10 Gbps (USB 3.1+)";
    default:
        return "Unknown";
    }
}

}  // namespace libusb
