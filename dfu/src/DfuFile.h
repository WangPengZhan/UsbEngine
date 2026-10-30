#pragma once
#include <stdint.h>
#include <string>
#include <ios>

namespace dfu
{
constexpr int kDfuSuffixLength = 16;

#pragma pack(push, 1)

struct DfuSuffix
{
    uint16_t bcdDevice{0xffff};
    uint16_t idProduct{0xffff};
    uint16_t idVendor{0xffff};
    uint16_t bcdDFU{0x011a};
    char ucDfuSignature[3]{};  // "UFD" in file byte order
    uint8_t bLength{};         // always 16
    uint32_t dwCRC{};          // CRC32 of whole image except this CRC
};

#pragma pack(pop)

class DfuFile
{
public:
    DfuFile() = default;
    DfuFile(std::string filename);

    bool isValid() const;

    const DfuSuffix& suffix() const;

    static DfuFile parseDfuFile(const std::string& filename);
    static bool addDfuSuffix(const std::string& filename, DfuSuffix suffix);  // ignore crc, comuplate in function

private:
    std::string filename_;
    std::streamoff size_{};
    DfuSuffix suffix_;
    bool crcValid_{};
};

}  // namespace dfu
