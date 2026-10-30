#include "UsbDevice.h"

#include <sstream>
#include <algorithm>

namespace libusb
{

UsbDevice::UsbDevice(uint16_t vid, uint16_t pid)
    : vid_(vid)
    , pid_(pid)
    , context_(makeUsbContext())
{
    if (!context_)
    {
        errMsg_ = "Failed to initialize libusb context";
        return;
    }
    libusb_device_handle* handle = libusb_open_device_with_vid_pid(context_.get(), vid, pid);
    if (!handle)
    {
        std::stringstream ss;
        ss << "Cannot open device VID=0x" << std::hex << vid << " PID=0x" << pid;
        errMsg_ = ss.str();
        return;
    }
    handle_.reset(handle);
    libusb_device* dev = libusb_get_device(handle_.get());
    speed_ = static_cast<libusb_speed>(libusb_get_device_speed(dev));
    libusb_device_descriptor desc{};
    int ret = libusb_get_device_descriptor(dev, &desc);
    if (ret != 0)
    {
        errMsg_ = std::string("Failed to read device descriptor: ") + libusb_error_name(ret);
        handle_.reset();
        return;
    }

    max_packet_size_ = std::max<uint16_t>(max_packet_size_, desc.bMaxPacketSize0);
    char buf[256] = {};
    if (desc.iProduct)
    {
        int length = libusb_get_string_descriptor_ascii(handle, desc.iProduct, reinterpret_cast<unsigned char*>(buf), sizeof(buf));
        if (length > 0)
        {
            name_.assign(buf, length);
        }
    }
}

UsbDevice::~UsbDevice()
{
}

bool UsbDevice::valid() const
{
    return bool(handle_);
}

uint16_t UsbDevice::vid() const
{
    return vid_;
}

uint16_t UsbDevice::pid() const
{
    return pid_;
}

const std::string& UsbDevice::name() const
{
    return name_;
}

libusb_device_handle* UsbDevice::handle() const
{
    return handle_.get();
}

const std::string& UsbDevice::errorMessage() const
{
    return errMsg_;
}

size_t UsbDevice::chunkSize() const
{
    size_t base = kMaxPacketSizeFs;
    if (speed_ == LIBUSB_SPEED_SUPER_PLUS)
    {
        base = kMaxPacketsizeSPs;
    }
    else if (speed_ == LIBUSB_SPEED_SUPER)
    {
        base = kMaxPacketsizeSs;
    }
    else if (speed_ == LIBUSB_SPEED_HIGH)
    {
        base = kMaxPacketSizeHs;
    }
    if (max_packet_size_ == 0)
    {
        return base;
    }
    size_t chunks = base / max_packet_size_;
    chunks = (chunks == 0 ? 1 : chunks);
    size_t adjusted = chunks * max_packet_size_;
    return adjusted;
}

std::string UsbDevice::speedInfo() const
{
    return speed_str(speed_);
}

uint16_t UsbDevice::ep0MaxPacketSize() const
{
    return max_packet_size_;
}

}  // namespace libusb
