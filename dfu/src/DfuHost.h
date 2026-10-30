#pragma once
#include <vector>
#include <string>
#include <functional>
#include <memory>

#include "DfuCommon.h"
#include "DfuUsbDevice.h"

namespace dfu
{
class DfuHost
{
public:
    DfuHost(uint16_t vid, uint16_t pid);
    ~DfuHost();

    bool detach(int timeout);

    int download(const uint8_t* data, uint16_t length);
    bool download(const std::vector<uint8_t>& buffer);
    bool download(const std::string& filename, std::function<void(double)> callback = {});
    bool downloadFinished();
    bool waitSyncState();

    int upload(uint8_t* data, uint16_t length);
    bool upload(std::vector<uint8_t>& buffer);
    bool upload(const std::string& filename, std::function<void(int)> callback = {});

    DfuStatusData getStatus();
    bool clearStatus();
    DfuState getState();

    bool abort();

    int timeout() const;
    void setTimeout(int time);
    // Use the runtime VID/PID when they differ from the DFU-mode IDs.
    void setTargetIdentity(uint16_t vid, uint16_t pid);

    const std::string& errorMessage() const;

private:
    uint16_t transSize() const;
    size_t adjustChunkSize() const;

private:
    std::unique_ptr<libusb::DfuUsbDevice> device_;
    int timeout_{3000};
    std::string errorMessage_;
    uint16_t transaction_{};
    uint16_t targetVid_{};
    uint16_t targetPid_{};
};
}  // namespace dfu
