#pragma once

#include <cstdint>
#include <string_view>
#include <vector>
#include <cstddef>

namespace emulator {

// границы диапазона включаются в трассировку
struct TickRange {
    std::uint64_t begin = 0;
    std::uint64_t end = 0;
};

class TickRangeFilter {
public:
    void setRanges(std::vector<TickRange> ranges);
    bool contains(std::uint64_t tick) noexcept;
    void reset() noexcept;
private:
    std::vector<TickRange> ranges_;
    std::size_t currentRange_ = 0;
};

std::vector<TickRange> parseTickRanges(std::string_view text);

}
