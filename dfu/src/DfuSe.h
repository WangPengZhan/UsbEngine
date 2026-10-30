#pragma once
#include <stdint.h>
#include <list>
#include <string>

namespace dfu
{
struct MemSegmentInfo
{
    uint32_t start{};
    uint32_t end{};
    uint32_t pagesize{};
    uint32_t memtype{};
};

using MemSegmentList = std::list<MemSegmentInfo>;
MemSegmentList parseMemoryLayout(const std::string& intfDescription);

}  // namespace dfu
