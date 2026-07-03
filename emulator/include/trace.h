#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace emulator {

// границы диапазона включаются в трассировку
struct TickRange {
    std::uint64_t begin = 0;
    std::uint64_t end = 0;
};

std::vector<TickRange> parseTickRanges(std::string_view text);
bool containsTick(const std::vector<TickRange>& ranges, std::uint64_t tick) noexcept;

}
