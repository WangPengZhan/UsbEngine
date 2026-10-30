#include "UsbClient.h"

namespace fastboot
{
USBClient::USBClient(uint16_t vid, uint16_t pid)
{
    connect(vid, pid);
}

USBClient::~USBClient() = default;

bool USBClient::connect(uint16_t vid, uint16_t pid)
{
    device_ = std::make_unique<libusb::FastbootUsbDevice>(vid, pid);
    if (!device_->valid())
    {
        errMsg_ = device_->errorMessage();
        return false;
    }
    return true;
}

int64_t USBClient::write(const uint8_t* data, int len)
{
    if (!isValid())
    {
        return -1;
    }
    int transferred = 0;
    int ret = libusb_bulk_transfer(device_->handle(), device_->EpOut(), const_cast<unsigned char*>(data), static_cast<int>(len), &transferred, timeoutMs_);
    if (ret != 0)
    {
        errMsg_ = libusb_error_name(ret);
        return -1;
    }
    return static_cast<size_t>(transferred);
}

int64_t USBClient::read(uint8_t* data, int len, int timeout)
{
    if (!isValid())
    {
        return -1;
    }
    int transferred = 0;
    int ret = libusb_bulk_transfer(device_->handle(), device_->EpIn(), data, static_cast<int>(len), &transferred, timeout);
    if (ret != 0)
    {
        errMsg_ = libusb_error_name(ret);
        return -1;
    }
    return static_cast<size_t>(transferred);
}

int USBClient::timeoutMs() const
{
    return timeoutMs_;
}

void USBClient::setTimeoutMs(int ms)
{
    timeoutMs_ = ms;
}

bool USBClient::isValid() const
{
    return device_ && device_->valid();
}

const std::string& USBClient::lastError() const
{
    return errMsg_;
}

size_t USBClient::chunkSize() const
{
    return isValid() ? device_->chunkSize() : 0;
}

}  // namespace fastboot
