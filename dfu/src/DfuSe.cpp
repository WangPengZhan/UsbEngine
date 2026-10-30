#include "DfuSe.h"
#include <sstream>
#include <regex>

namespace dfu
{
constexpr int kDfuSeReadable = 1;
constexpr int kDfuSeErasable = 2;
constexpr int kDfuSeWriteable = 4;

static uint32_t parseMultiplier(char c)
{
    switch (c)
    {
    case 'B':
        return 1;
    case 'K':
        return 1024;
    case 'M':
        return 1024 * 1024;
    default:
        return 1;  // fallback
    }
}

static uint8_t parseMemType(char c)
{
    switch (c)
    {
    case 'a':
        return kDfuSeReadable;
    case 'b':
        return kDfuSeErasable;
    case 'c':
        return kDfuSeReadable | kDfuSeErasable;
    case 'd':
        return kDfuSeWriteable;
    case 'e':
        return kDfuSeReadable | kDfuSeWriteable;
    case 'f':
        return kDfuSeErasable | kDfuSeWriteable;
    case 'g':
        return kDfuSeReadable | kDfuSeErasable | kDfuSeWriteable;
    default:
        return 0;
    }
}

MemSegmentList parseMemoryLayout(const std::string& intfDescription)
{
    MemSegmentList result;
    auto pos = intfDescription.find('@');
    if (pos == std::string::npos)
    {
        return result;
    }

    auto slash = intfDescription.find('/', pos + 1);
    if (slash == std::string::npos)
    {
        return result;
    }

    const std::string layout = intfDescription.substr(slash + 1);
    if (layout.empty() || layout.back() == '/')
    {
        return {};
    }

    std::stringstream sections(layout);
    std::string addressText;
    std::string segmentText;
    const std::regex segmentPattern(R"(^\s*([0-9]+)\*([0-9]+)\s*([BKM]?)\s*([a-g])\s*$)");
    while (std::getline(sections, addressText, '/'))
    {
        if (!std::getline(sections, segmentText, '/') || addressText.empty() || segmentText.empty() || segmentText.back() == ',')
        {
            return {};
        }

        uint64_t address = 0;
        try
        {
            size_t used = 0;
            address = std::stoull(addressText, &used, 16);
            if (used != addressText.size() || address > UINT32_MAX)
            {
                return {};
            }
        }
        catch (...)
        {
            return {};
        }

        std::stringstream segments(segmentText);
        std::string segment;
        while (std::getline(segments, segment, ','))
        {
            std::smatch match;
            if (!std::regex_match(segment, match, segmentPattern))
            {
                return {};
            }

            try
            {
                uint64_t count = std::stoull(match[1].str());
                uint64_t unit = std::stoull(match[2].str());
                uint64_t scale = match[3].str().empty() ? 1 : parseMultiplier(match[3].str()[0]);
                if (unit > UINT32_MAX / scale)
                {
                    return {};
                }
                uint64_t pageSize = unit * scale;
                if (pageSize == 0 || count > UINT32_MAX / pageSize)
                {
                    return {};
                }
                uint64_t length = count * pageSize;
                if (count == 0 || address + length > static_cast<uint64_t>(UINT32_MAX) + 1)
                {
                    return {};
                }
                result.push_back({static_cast<uint32_t>(address), static_cast<uint32_t>(address + length - 1), static_cast<uint32_t>(pageSize),
                                  parseMemType(match[4].str()[0])});
                address += length;
            }
            catch (...)
            {
                return {};
            }
        }
    }
    return result;
}

}  // namespace dfu
