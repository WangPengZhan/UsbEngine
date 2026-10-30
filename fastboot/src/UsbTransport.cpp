#include "UsbTransport.h"

#include <iostream>

#include "FastbootCommon.h"
#include "UsbClient.h"

namespace fastboot
{

int64_t UsbTransport::read(std::vector<uint8_t>& buf)
{
    if (!client_)
    {
        return -1;
    }

    buf.resize(kResultMaxLength);
    int transferred = client_->read(buf.data(), static_cast<int>(buf.size()), client_->timeoutMs());
    if (transferred < 0)
    {
        std::cout << "read error: " << client_->lastError() << std::endl;
        return -1;
    }

    buf.resize(transferred);
    return transferred;
}

int64_t UsbTransport::write(const std::vector<uint8_t>& data)
{
    if (!client_)
    {
        return -1;
    }
    size_t chunksize = client_->chunkSize();
    if (chunksize == 0)
    {
        return -1;
    }

    uint64_t offset = 0;
    while (offset < data.size())
    {
        int sendLen = std::min<uint64_t>(chunksize, data.size() - offset);
        int transferred = client_->write(data.data() + offset, sendLen);
        if (transferred != sendLen)
        {
            std::cout << "write error: " << client_->lastError() << std::endl;
            return -1;
        }
        offset += transferred;
    }
    return offset;
}

int UsbTransport::close()
{
    return 0;
}

int UsbTransport::reset()
{
    return 0;
}

void UsbTransport::setReadTimeout(int ms)
{
    if (client_)
        client_->setTimeoutMs(ms);
}

void UsbTransport::setClient(std::unique_ptr<USBClient> client)
{
    client_ = std::move(client);
}

bool UsbTransport::hasClient() const
{
    return bool(client_);
}

}  // namespace fastboot
