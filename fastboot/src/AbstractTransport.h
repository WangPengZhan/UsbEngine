#pragma once
#include <vector>

namespace fastboot
{
class AbstractTransport
{
public:
    virtual ~AbstractTransport() = default;

    virtual int64_t read(std::vector<uint8_t>& buf) = 0;
    virtual int64_t write(const std::vector<uint8_t>& data) = 0;
    virtual int close() = 0;
    virtual int reset() = 0;

    virtual void setReadTimeout(int ms)
    {
        (void)ms;
    }
};

}  // namespace fastboot
