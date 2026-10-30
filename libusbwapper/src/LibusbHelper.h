#pragma once
#include <memory>
#include <string>
#include <vector>

#include <libusb.h>

namespace libusb
{

constexpr int kMaxPacketSizeFs = 64 * 1024;         // 64KB
constexpr int kMaxPacketSizeHs = 128 * 1024;        // 128KB
constexpr int kMaxPacketsizeSs = 1024 * 1024;       // 1M
constexpr int kMaxPacketsizeSPs = 2 * 1024 * 1024;  // 2M

struct LibUsbDeleter
{
    void operator()(libusb_context* ctx) const
    {
        if (ctx)
        {
            libusb_exit(ctx);
        }
    }

    void operator()(libusb_device_handle* handle) const
    {
        if (handle)
        {
            libusb_close(handle);
        }
    }

    void operator()(libusb_config_descriptor* config) const
    {
        if (config)
        {
            libusb_free_config_descriptor(config);
        }
    }
};

using LibUsbContextPtr = std::unique_ptr<libusb_context, LibUsbDeleter>;
using LibUsbDeviceHandlePtr = std::unique_ptr<libusb_device_handle, LibUsbDeleter>;
using LibUsbConfigDescriptorPtr = std::unique_ptr<libusb_config_descriptor, LibUsbDeleter>;

LibUsbContextPtr makeUsbContext();
std::vector<std::pair<uint32_t, uint32_t>> findDFUDevices();
std::vector<std::pair<uint32_t, uint32_t>> findFastbootDevices();
std::string speed_str(libusb_speed speed);

}  // namespace libusb
