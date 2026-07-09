#pragma once

#include <cstdint>
#include <string_view>
#include <vector>
#include <cstddef>

namespace emulator {

// границы диапазона включаются в трассировку
template <typename T>
struct Range {
    T begin = 0;
    T end = 0;
};

using TickRange = Range<std::uint64_t>;
using AddressRange = Range<std::uint32_t>;

template <typename T>
class RangeFilter {
public:
    void setRanges(std::vector<TickRange> ranges);
    bool contains(T elem) noexcept;
    void reset() noexcept;
private:
    std::vector<Range> ranges_;
};

std::vector<TickRange> parseTickRanges(std::string_view text);

}
