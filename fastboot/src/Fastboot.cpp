#include "Fastboot.h"

#include "FastbootCommon.h"

#include <chrono>
#include <sstream>
#include <iomanip>
#include <thread>
#include <fstream>
#include <iostream>
#include <filesystem>

namespace fastboot
{
const uint32_t MAX_DOWNLOAD_SIZE = 32 * 1024 * 1024;

FastBootHost::FastBootHost()
{
}

FastBootHost::FastBootHost(std::unique_ptr<AbstractTransport> transport)
{
    setTransport(std::move(transport));
}

FastBootHost::~FastBootHost()
{
}

void FastBootHost::setTransport(std::unique_ptr<AbstractTransport> transport)
{
    transport_ = std::move(transport);
}

RetCode FastBootHost::Boot(FastbootResponse& result)
{
    return sendCommand(CMD_BOOT, result);
}

RetCode FastBootHost::Continue(FastbootResponse& result)
{
    return sendCommand(CMD_CONTINUE, result);
}

RetCode FastBootHost::Download(const std::vector<uint8_t>& buf, FastbootResponse& result)
{
    error_.clear();
    if (buf.empty() || buf.size() > MAX_DOWNLOAD_SIZE)
    {
        error_ = "Buffer is too large or 0 bytes";
        return BAD_ARG;
    }
    if (RetCode ret = DownloadCommand(buf.size(), result))
    {
        return ret;
    }
    if (result.result != DATA || result.dsize != buf.size())
    {
        error_ = "Device did not accept the requested download size";
        return BAD_DEV_RESP;
    }

    // Write the buffer
    if (RetCode ret = sendBuffer(buf))
    {
        return ret;
    }
    // Wait for response
    RetCode finalResult = handleResponse(result, 30000);
    if (finalResult == SUCCESS && result.result != OKAY)
    {
        error_ = "Expected OKAY after download data";
        return BAD_DEV_RESP;
    }

    return finalResult;
}

RetCode FastBootHost::Download(const std::string& filename, FastbootResponse& result, std::function<void(double)> callback)
{
    error_.clear();
    uint32_t deviceLimit = getDownloadSize();
    if (deviceLimit == 0)
    {
        deviceLimit = MAX_DOWNLOAD_SIZE;
    }
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file)
    {
        error_ = "Failed to open file: " + filename;
        return FILE_ERROR;
    }
    std::streamsize totalSize = file.tellg();
    if (totalSize <= 0)
    {
        error_ = "File is empty: " + filename;
        return FILE_ERROR;
    }
    if (static_cast<uint64_t>(totalSize) > deviceLimit || static_cast<uint64_t>(totalSize) > UINT32_MAX)
    {
        error_ = "File exceeds the device download limit";
        return BAD_ARG;
    }

    file.seekg(0, std::ios::beg);
    RetCode commandResult = DownloadCommand(static_cast<uint32_t>(totalSize), result);
    if (commandResult != SUCCESS)
    {
        return commandResult;
    }

    if (result.result != DATA || result.dsize != static_cast<uint32_t>(totalSize))
    {
        error_ = "Device did not accept the requested download size";
        return BAD_DEV_RESP;
    }

    std::vector<uint8_t> buffer;
    size_t bytesRemaining = static_cast<size_t>(totalSize);
    size_t totalSent = 0;
#if _DEBUG
    auto start = std::chrono::steady_clock::now();
#endif
    while (bytesRemaining > 0)
    {
        size_t chunkSize = std::min<size_t>(bytesRemaining, 64 * 1024);
        buffer.resize(chunkSize);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), chunkSize))
        {
            error_ = "Failed to read file: " + filename;
            return IO_ERROR;
        }

        RetCode ret = sendBuffer(buffer);
        if (ret != SUCCESS)
        {
            error_ = "Download chunk failed at offset " + std::to_string(totalSent);
            return ret;
        }

        totalSent += chunkSize;
        bytesRemaining -= chunkSize;

        double progress = (std::min)(99.0, (static_cast<double>(totalSent) / totalSize) * 100.0);
        if (callback)
        {
            callback(progress);
        }
        std::cout << "\r[FastBoot] Download progress: " << std::fixed << std::setprecision(1) << progress << "%" << std::endl;
    }

    RetCode finalResult = handleResponse(result, 30000);
    if (finalResult != SUCCESS || result.result != OKAY)
    {
        if (finalResult == SUCCESS)
        {
            error_ = "Expected OKAY after download data";
        }
        return finalResult == SUCCESS ? BAD_DEV_RESP : finalResult;
    }

#if _DEBUG
    auto end = std::chrono::steady_clock::now();
    double seconds = std::chrono::duration<double>(end - start).count();
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "[FastBoot] Download time: " << seconds << " s" << std::endl;
    auto fileSize = std::filesystem::file_size(filename);
    double speed = (fileSize / 1'000'000.0) / seconds;
    std::cout << "[FastBoot] Speed: " << speed << " MB/s" << std::endl;
#endif

    if (callback)
    {
        callback(100.0);
    }
    return SUCCESS;
}

RetCode FastBootHost::Erase(const std::string& partition, FastbootResponse& result)
{
    std::string cmd = CMD_ERASE + std::string(":") + partition;
    return sendCommand(cmd, result);
}

RetCode FastBootHost::Flash(const std::string& partition, FastbootResponse& result)
{
    std::string cmd = CMD_FLASH + std::string(":") + partition;
    return sendCommand(cmd, result);
}

RetCode FastBootHost::GetVar(const std::string& key, FastbootResponse& result)
{
    std::string cmd = CMD_GETVAR + std::string(":") + key;
    return sendCommand(cmd, result);
}

RetCode FastBootHost::GetVarAll(FastbootResponse& result)
{
    return GetVar("all", result);
}

RetCode FastBootHost::Reboot(FastbootResponse& result)
{
    return sendCommand(CMD_REBOOT, result);
}

RetCode FastBootHost::RebootTo(std::string target, FastbootResponse& result)
{
    std::string cmd = "reboot-" + target;
    return sendCommand(cmd, result);
}

RetCode FastBootHost::DownloadCommand(uint32_t size, FastbootResponse& result)
{
    std::stringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << size;
    std::string cmd = CMD_DOWNLOAD + std::string(":") + ss.str();
    if (RetCode ret = sendCommand(cmd, result))
    {
        return ret;
    }
    return SUCCESS;
}

RetCode FastBootHost::sendCommand(const std::string& cmd, FastbootResponse& response)
{
    error_.clear();
    if (!transport_)
    {
        error_ = "No transport configured";
        return BAD_ARG;
    }

    if (cmd.size() > kCMDMaxLength)
    {
        error_ = "Command length to RawCommand() is too long";
        return BAD_ARG;
    }

    std::cout << "request ===>: " << cmd << std::endl;
    if (transport_->write(std::vector<uint8_t>(cmd.begin(), cmd.end())) != static_cast<int>(cmd.size()))
    {
        error_ = "Write to device failed";
        return IO_ERROR;
    }

    const bool longOperation = cmd.rfind("flash:", 0) == 0 || cmd.rfind("erase:", 0) == 0;
    return handleResponse(response, longOperation ? 120000 : 3000);
}

uint32_t FastBootHost::getDownloadSize()
{
    uint32_t result{};
    FastbootResponse response;
    if (RetCode res = GetVar("max-download-size", response))
    {
        return 0;
    }
    try
    {
        auto size = std::stoull(response.message, nullptr, 16);
        if (size <= UINT32_MAX)
        {
            result = static_cast<uint32_t>(size);
        }
    }
    catch (...)
    {
    }
    return result;
}

const std::string& FastBootHost::errorMessage() const
{
    return error_;
}

RetCode FastBootHost::handleResponse(FastbootResponse& result, int timeoutMs)
{
    if (!transport_)
    {
        error_ = "No transport configured";
        return BAD_ARG;
    }

    transport_->setReadTimeout(timeoutMs);
    auto start = std::chrono::steady_clock::now();
    while ((std::chrono::steady_clock::now() - start) < std::chrono::milliseconds(timeoutMs))
    {
        std::vector<uint8_t> response;
        int64_t r = transport_->read(response);
        if (r < 0)
        {
            error_ = "Status read failed";
            return IO_ERROR;
        }
        if (r == 0 || static_cast<size_t>(r) > response.size())
        {
            error_ = "Invalid status response length";
            return BAD_DEV_RESP;
        }

        std::string strResponse(response.begin(), response.begin() + r);
        std::cout << "response <===: " << strResponse << std::endl;
        result = FastbootResponse::parse(strResponse);
        if ((result.result == INFO) || (result.result == TEXT))
        {
            start = std::chrono::steady_clock::now();
        }
        else if ((result.result == OKAY) || (result.result == DATA))
        {
            return SUCCESS;
        }
        else if (result.result == FAIL)
        {
            return DEVICE_FAIL;
        }
        else
        {
            error_ = std::string("Device sent unknown status code: ") + strResponse;
            return BAD_DEV_RESP;
        }
    }

    error_ = "Timed out waiting for device status";
    return TIMEOUT;
}

RetCode FastBootHost::sendBuffer(const std::vector<uint8_t>& buf)
{
    if (buf.empty())
    {
        return BAD_ARG;
    }
    int64_t tmp = transport_->write(buf);
    if (tmp < 0)
    {
        error_ = "Write to device failed in sendBuffer()";
        return IO_ERROR;
    }
    else if (static_cast<size_t>(tmp) != buf.size())
    {
        error_ = "Failed to write all data, data len: " + std::to_string(buf.size()) + ", send len: " + std::to_string(tmp);
        return IO_ERROR;
    }
    return SUCCESS;
}

}  // namespace fastboot
