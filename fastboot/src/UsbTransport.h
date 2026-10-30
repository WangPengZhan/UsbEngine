#pragma once
#include <memory>

#include "AbstractTransport.h"

namespace fastboot
{
class USBClient;

class UsbTransport : public AbstractTransport
{
public:
    UsbTransport() = default;
    ~UsbTransport() = default;

    int64_t read(std::vector<uint8_t>& buf) override;
    int64_t write(const std::vector<uint8_t>& data) override;
    int close() override;
    int reset() override;
    void setReadTimeout(int ms) override;

    void setClient(std::unique_ptr<USBClient> client);
    bool hasClient() const;

private:
    std::unique_ptr<USBClient> client_;
};
}  // namespace fastboot
