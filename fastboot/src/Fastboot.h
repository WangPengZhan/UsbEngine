#pragma once
#include <string>
#include <functional>
#include <memory>
#include <vector>

#include "AbstractTransport.h"
#include "FastbootCommon.h"

namespace fastboot
{
enum RetCode
{
    SUCCESS = 0,
    IO_ERROR,
    BAD_ARG,
    FILE_ERROR,
    TIMEOUT,
    UNKNOWN_ERROR,
    DEVICE_FAIL,
    BAD_DEV_RESP,
};

class FastBootHost
{
public:
    FastBootHost();
    FastBootHost(std::unique_ptr<AbstractTransport> transport);
    ~FastBootHost();

    void setTransport(std::unique_ptr<AbstractTransport> transport);

    RetCode Boot(FastbootResponse& result);
    RetCode Continue(FastbootResponse& result);
    RetCode Download(const std::vector<uint8_t>& buf, FastbootResponse& result);
    RetCode Download(const std::string& filename, FastbootResponse& result, std::function<void(double)> callback = {});
    RetCode Erase(const std::string& partition, FastbootResponse& result);
    RetCode Flash(const std::string& partition, FastbootResponse& result);
    RetCode GetVar(const std::string& key, FastbootResponse& result);
    RetCode GetVarAll(FastbootResponse& result);
    RetCode Reboot(FastbootResponse& result);
    RetCode RebootTo(std::string target, FastbootResponse& result);

    uint32_t getDownloadSize();
    void setDownloadSize(uint32_t size);
    const std::string& errorMessage() const;

private:
    RetCode DownloadCommand(uint32_t size, FastbootResponse& result);
    RetCode sendCommand(const std::string& cmd, FastbootResponse& response);
    RetCode sendBuffer(const std::vector<uint8_t>& buf);
    RetCode handleResponse(FastbootResponse& result, int timeoutMs = 3000);

private:
    std::unique_ptr<AbstractTransport> transport_;
    std::string error_;
};

}  // namespace fastboot
