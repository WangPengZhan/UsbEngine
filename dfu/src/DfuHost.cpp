#include "DfuHost.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <thread>
#include <limits>
#include <cstring>

#include "DfuFile.h"

namespace dfu
{
constexpr uint16_t kDefaultPacketSize = 1024;
constexpr uint16_t kMaxPacketSize = 16384;
constexpr uint32_t kDefaultChunkSize = 1024 * 1024;

DfuHost::DfuHost(uint16_t vid, uint16_t pid)
    : device_(std::make_unique<libusb::DfuUsbDevice>(vid, pid))
    , targetVid_(vid)
    , targetPid_(pid)
{
}

DfuHost::~DfuHost()
{
}

bool DfuHost::detach(int timeout)
{
    if (!device_ || !device_->valid())
    {
        return false;
    }
    int ret =
        libusb_control_transfer(device_->handle(), /* bmRequestType */ LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
                                /* bRequest */ DFU_DETACH, /* wValue */ timeout, /* wIndex */ device_->interface(), /* Data */ NULL, /* wLength */ 0, timeout_);
    if (ret != 0)
    {
        errorMessage_ = libusb_error_name(ret);
    }

    return ret == 0;
}

int DfuHost::download(const uint8_t* data, uint16_t length)
{
    if (!device_ || !device_->valid())
    {
        return -1;
    }
    if ((0 == length) || (nullptr == data))
    {
        return -1;
    }
    if (transaction_ == (std::numeric_limits<uint16_t>::max)())
    {
        errorMessage_ = "DFU block number exhausted";
        return -1;
    }

    int ret = libusb_control_transfer(device_->handle(), /* bmRequestType */ LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
                                      /* bRequest */ DFU_DNLOAD, /* wValue */ transaction_, /* wIndex */ device_->interface(),
                                      /* Data */ (unsigned char*)data, /* wLength */ length, timeout_);
    if (ret < 0)
    {
        errorMessage_ = libusb_error_name(ret);
    }
    else if (ret == length)
    {
        ++transaction_;
    }

    return ret;
}

bool DfuHost::download(const std::vector<uint8_t>& buffer)
{
    if (!device_ || !device_->valid())
    {
        return false;
    }

    const uint8_t* data = buffer.data();
    size_t total = buffer.size();
    size_t offset = 0;
    uint16_t downloadSize = transSize();
    while (offset < total)
    {
        size_t chunk = std::min<size_t>(downloadSize, total - offset);
        int ret = download(data + offset, static_cast<uint16_t>(chunk));
        if (ret < 0)
        {
            std::cerr << "DFU_DNLOAD failed at offset " << offset << ": " << libusb_error_name(ret) << std::endl;
            return false;
        }
        if (ret != static_cast<int>(chunk))
        {
            std::cerr << "DFU_DNLOAD short write " << ret << " of " << chunk << std::endl;
            return false;
        }
        offset += chunk;
        DfuStatusData statusData;
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        do
        {
            statusData = getStatus();
            if (statusData.bStatus != OK)
            {
                errorMessage_ = "DFU device reported an error";
                return false;
            }

            auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
            auto poll = std::chrono::milliseconds((device_->quirks() & libusb::quirkPollTimeout) ? libusb::defaultPollTimeout : statusData.bwPollTimeout);
            if (remaining > std::chrono::milliseconds::zero())
            {
                std::this_thread::sleep_for((std::min)(poll, remaining));
            }
            if (std::chrono::steady_clock::now() >= deadline)
            {
                errorMessage_ = "Timed out waiting for DFU download state";
                return false;
            }
        } while (statusData.bState != DFU_DNLOAD_IDLE && statusData.bState != DFU_ERROR);

        if (statusData.bStatus != OK || statusData.bState == DFU_ERROR)
        {
            errorMessage_ = "DFU device entered an error state";
            return false;
        }
    }
    return true;
}

bool DfuHost::download(const std::string& filename, std::function<void(double)> callback)
{
    if (!device_ || !device_->valid())
    {
        return false;
    }
    errorMessage_.clear();
    size_t downloadSize = adjustChunkSize();
    DfuFile dfuFile = DfuFile::parseDfuFile(filename);
    if (std::memcmp(dfuFile.suffix().ucDfuSignature, "UFD", 3) == 0 && !dfuFile.isValid())
    {
        errorMessage_ = "Invalid DFU file suffix or CRC";
        return false;
    }
    if (dfuFile.isValid())
    {
        const auto& suffix = dfuFile.suffix();
        if (suffix.idVendor != 0xffff && (suffix.idVendor != targetVid_ || (suffix.idProduct != 0xffff && suffix.idProduct != targetPid_)))
        {
            errorMessage_ = "DFU file targets a different device";
            return false;
        }
    }
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file)
    {
        errorMessage_ = "Failed to open file: " + filename;
        return false;
    }
    file.seekg(0, std::ios::beg);
    char prefix[5] = {};
    if (file.read(prefix, sizeof(prefix)) && std::memcmp(prefix, "DfuSe", sizeof(prefix)) == 0)
    {
        errorMessage_ = "DfuSe container download is not implemented";
        return false;
    }
    file.clear();
    file.seekg(0, std::ios::end);
    std::streamsize totalSize = file.tellg();
    if (totalSize <= 0)
    {
        errorMessage_ = "File is empty: " + filename;
        return false;
    }
    file.seekg(0, std::ios::beg);
    auto st = getStatus();
    if (st.bState == DFU_ERROR)
    {
        if (!clearStatus())
            return false;
    }
    std::vector<uint8_t> buffer;
    buffer.reserve(downloadSize);
    size_t bytesRemaining = static_cast<size_t>(totalSize) - (dfuFile.isValid() ? kDfuSuffixLength : 0);
    if (bytesRemaining == 0)
    {
        errorMessage_ = "Firmware payload is empty";
        return false;
    }
    size_t totalSent = 0;
    while (bytesRemaining > 0)
    {
        size_t chunkSize = std::min<size_t>(bytesRemaining, downloadSize);
        buffer.resize(chunkSize);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), chunkSize))
        {
            errorMessage_ = "Failed to read file: " + filename;
            return false;
        }
        bool ret = download(buffer);
        if (!ret)
        {
            errorMessage_ = "Download chunk failed at offset: " + std::to_string(totalSent);
            return ret;
        }
        totalSent += chunkSize;
        bytesRemaining -= chunkSize;
        double progress = (std::min)(99.0, (static_cast<double>(totalSent) / (totalSize - (dfuFile.isValid() ? kDfuSuffixLength : 0))) * 100.0);
        if (callback)
        {
            callback(progress);
        }
        std::cout << "\r[DFU] Download progress: " << std::fixed << std::setprecision(1) << progress << "%" << std::endl;
    }
    if (!downloadFinished())
    {
        return false;
    }
    if (!waitSyncState())
        return false;
    if (callback)
    {
        callback(100.0);
    }
    return true;
}

bool DfuHost::downloadFinished()
{
    if (!device_ || !device_->valid())
    {
        return false;
    }
    if (transaction_ == (std::numeric_limits<uint16_t>::max)())
    {
        errorMessage_ = "DFU block number exhausted";
        return false;
    }
    int ret = libusb_control_transfer(device_->handle(), /* bmRequestType */ LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
                                      /* bRequest */ DFU_DNLOAD, /* wValue */ transaction_, /* wIndex */ device_->interface(), /* Data */ NULL,
                                      /* wLength */ 0, timeout_);
    if (ret < 0)
    {
        errorMessage_ = libusb_error_name(ret);
        return false;
    }
    if (ret != 0)
    {
        errorMessage_ = "Unexpected DFU completion response length";
        return false;
    }
    ++transaction_;
    return true;
}

bool DfuHost::waitSyncState()
{
    if (!device_ || !device_->valid())
        return false;
    DfuStatusData statusData;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        statusData = getStatus();
        if (statusData.bStatus != OK)
            return false;
        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
        auto poll = std::chrono::milliseconds((device_->quirks() & libusb::quirkPollTimeout) ? libusb::defaultPollTimeout : statusData.bwPollTimeout);
        if (remaining > std::chrono::milliseconds::zero())
            std::this_thread::sleep_for((std::min)(poll, remaining));
        if (std::chrono::steady_clock::now() >= deadline)
        {
            errorMessage_ = "Timed out waiting for DFU manifestation";
            return false;
        }
    } while ((statusData.bState == DFU_MANIFEST_SYNC) || (statusData.bState == DFU_MANIFEST));
    return statusData.bStatus == OK && (statusData.bState == DFU_IDLE || statusData.bState == DFU_MANIFEST_WAIT_RST);
}

int DfuHost::upload(uint8_t* data, uint16_t length)
{
    if (!device_ || !device_->valid())
    {
        return -1;
    }
    if ((0 == length) || (nullptr == data))
    {
        return -1;
    }
    if (transaction_ == (std::numeric_limits<uint16_t>::max)())
    {
        errorMessage_ = "DFU block number exhausted";
        return -1;
    }
    int ret = libusb_control_transfer(device_->handle(), /* bmRequestType */ LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
                                      /* bRequest */ DFU_UPLOAD, /* wValue */ transaction_, /* wIndex */ device_->interface(),
                                      /* Data */ (unsigned char*)data, /* wLength */ length, timeout_);
    if (ret < 0)
    {
        errorMessage_ = libusb_error_name(ret);
    }
    else
    {
        ++transaction_;
    }
    return ret;
}

bool DfuHost::upload(std::vector<uint8_t>& buffer)
{
    if (!device_ || !device_->valid())
    {
        return false;
    }
    uint32_t uploadSize = transSize();
    auto buff = std::make_unique<uint8_t[]>(uploadSize);
    while (1)
    {
        int ret = upload(buff.get(), uploadSize);
        if (ret < 0)
        {
            return false;
        }

        buffer.insert(buffer.end(), buff.get(), buff.get() + ret);
        if (ret < uploadSize)
        {
            break;
        }
    }
    return true;
}

bool DfuHost::upload(const std::string& filename, std::function<void(int)> callback)
{
    if (!device_ || !device_->valid())
    {
        return false;
    }
    errorMessage_.clear();
    uint32_t uploadSize = transSize();
    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file)
    {
        errorMessage_ = "Failed to open file: " + filename;
        return false;
    }

    auto buff = std::make_unique<uint8_t[]>(uploadSize);
    int totalByte = 0;
    while (1)
    {
        int ret = upload(buff.get(), uploadSize);
        if (ret < 0)
        {
            return false;
        }

        file.write((char*)buff.get(), ret);
        if (!file)
        {
            errorMessage_ = "Failed to write file: " + filename;
            return false;
        }

        totalByte += ret;
        if (callback)
        {
            callback(totalByte);
        }
        if (ret < uploadSize)
        {
            break;
        }
    }

    return static_cast<bool>(file.flush());
}

DfuStatusData DfuHost::getStatus()
{
    DfuStatusData data;
    data.bStatus = ERROR_UNKNOWN;
    data.bwPollTimeout = 0;
    data.bState = DFU_ERROR;
    data.iString = 0;
    if (!device_ || !device_->valid())
    {
        return data;
    }
    unsigned char buffer[6] = {};
    int ret =
        libusb_control_transfer(device_->handle(), /* bmRequestType */ LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
                                /* bRequest */ DFU_GETSTATUS, /* wValue */ 0, /* wIndex */ device_->interface(), /* Data */ buffer, /* wLength */ 6, timeout_);
    if (6 == ret)
    {
        data.bStatus = buffer[0];
        data.bwPollTimeout = ((0xff & buffer[3]) << 16) | ((0xff & buffer[2]) << 8) | (0xff & buffer[1]);
        data.bState = buffer[4];
        data.iString = buffer[5];
    }
    return data;
}

bool DfuHost::clearStatus()
{
    if (!device_ || !device_->valid())
    {
        return false;
    }
    int ret =
        libusb_control_transfer(device_->handle(), /* bmRequestType */ LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
                                /* bRequest */ DFU_CLRSTATUS, /* wValue */ 0, /* wIndex */ device_->interface(), /* Data */ NULL, /* wLength */ 0, timeout_);
    if (ret != 0)
    {
        errorMessage_ = libusb_error_name(ret);
    }
    else
    {
        transaction_ = 0;
    }

    return ret == 0;
}

DfuState DfuHost::getState()
{
    DfuState result = DFU_ERROR;
    unsigned char buffer[1];
    if (!device_ || !device_->valid())
    {
        return result;
    }
    int ret =
        libusb_control_transfer(device_->handle(), /* bmRequestType */ LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
                                /* bRequest */ DFU_GETSTATE, /* wValue */ 0, /* wIndex */ device_->interface(), /* Data */ buffer, /* wLength */ 1, timeout_);
    if (ret < 1)
    {
        return result;
    }
    return static_cast<DfuState>(buffer[0]);
}

bool DfuHost::abort()
{
    if (!device_ || !device_->valid())
    {
        return false;
    }
    int ret = libusb_control_transfer(device_->handle(), /* bmRequestType */ LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
                                      /* bRequest */ DFU_ABORT, /* wValue */ 0, /* wIndex */ device_->interface(), /* Data */ NULL, /* wLength */ 0, timeout_);
    if (ret != 0)
    {
        errorMessage_ = libusb_error_name(ret);
    }
    else
    {
        transaction_ = 0;
    }
    return ret == 0;
}

int DfuHost::timeout() const
{
    return timeout_;
}

void DfuHost::setTimeout(int time)
{
    if (time > 0)
    {
        timeout_ = time;
    }
}

void DfuHost::setTargetIdentity(uint16_t vid, uint16_t pid)
{
    targetVid_ = vid;
    targetPid_ = pid;
}

const std::string& DfuHost::errorMessage() const
{
    return errorMessage_;
}

uint16_t DfuHost::transSize() const
{
    uint16_t downloadSize = device_->transSize();
    if (downloadSize == 0)
    {
        downloadSize = kDefaultPacketSize;
    }
    if (downloadSize > kMaxPacketSize)
    {
        downloadSize = kMaxPacketSize;
    }
    return downloadSize;
}

size_t DfuHost::adjustChunkSize() const
{
    size_t chunkSize = device_->chunkSize();
    if (chunkSize == 0)
    {
        chunkSize = kDefaultChunkSize;
    }
    int num = chunkSize / transSize();
    num = (num == 0) ? 1 : num;
    return transSize() * num;
}

}  // namespace dfu
