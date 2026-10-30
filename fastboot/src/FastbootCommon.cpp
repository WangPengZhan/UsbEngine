#include "FastbootCommon.h"

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace fastboot
{
std::string resultToString(FastbootResult result)
{
    switch (result)
    {
    case OKAY:
        return OKAY_STR;
    case FAIL:
        return FAIL_STR;
    case INFO:
        return INFO_STR;
    case DATA:
        return DATA_STR;
    case TEXT:
        return TEXT_STR;
    default:
        return "UNKNOWN";
    }
}

FastbootResult stringToResult(const std::string& result)
{
    std::string normal = result;
    std::transform(normal.begin(), normal.end(), normal.begin(), ::toupper);
    if (normal == OKAY_STR)
    {
        return OKAY;
    }
    else if (normal == FAIL_STR)
    {
        return FAIL;
    }
    else if (normal == INFO_STR)
    {
        return INFO;
    }
    else if (normal == DATA_STR)
    {
        return DATA;
    }
    else if (normal == TEXT_STR)
    {
        return TEXT;
    }
    else
    {
        return FAIL;
    }
}

bool isValidResult(const std::string& result)
{
    std::string r = result;
    std::transform(r.begin(), r.end(), r.begin(), ::toupper);
    return (r == OKAY_STR || r == FAIL_STR || r == INFO_STR || r == DATA_STR);
}

std::string FastbootResponse::toString() const
{
    std::string text = resultToString(result);
    if (result == DATA)
    {
        std::stringstream ss;
        ss << std::hex << std::setw(8) << std::setfill('0') << dsize;
        text += ss.str();
    }
    else
    {
        text += message;
    }
    return text;
}

FastbootResponse FastbootResponse::parse(const std::string& content)
{
    FastbootResponse result;
    if (content.find(INFO_STR) == 0)
    {
        result.result = INFO;
        result.message = content.substr(strlen(INFO_STR));
    }
    else if (content.find(DATA_STR) == 0)
    {
        result.result = DATA;
        std::string tmp = content.substr(strlen(DATA_STR));
        if (tmp.size() != 8 || tmp.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
        {
            result.result = UNKOWN;
            return result;
        }

        result.dsize = static_cast<std::uint32_t>(std::stoul(tmp, nullptr, 16));
    }
    else if (content.find(OKAY_STR) == 0)
    {
        result.result = OKAY;
        result.message = content.substr(strlen(OKAY_STR));
    }
    else if (content.find(FAIL_STR) == 0)
    {
        result.result = FAIL;
        result.message = content.substr(strlen(FAIL_STR));
    }
    else if (content.find(TEXT_STR) == 0)
    {
        result.result = TEXT;
        result.message = content.substr(strlen(TEXT_STR));
    }
    else
    {
        result.message = content;
    }
    return result;
}

}  // namespace fastboot
