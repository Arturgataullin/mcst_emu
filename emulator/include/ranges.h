#pragma once

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace emulator {

template <typename T>
struct Range {
    T begin = 0;
    T end = 0;
};

using TickRange = Range<std::uint64_t>;
using AddressRange = Range<std::uint32_t>;

// логика диапазонов одинаковая для тиков и адресов, отличается только тип числа
template <typename T>
std::vector<Range<T>> normalizeRanges(std::vector<Range<T>> ranges) {
    std::sort(ranges.begin(), ranges.end(), [](const Range<T>& lhs, const Range<T>& rhs) {
        if (lhs.begin != rhs.begin) {
            return lhs.begin < rhs.begin;
        }
        return lhs.end < rhs.end;
    });

    std::vector<Range<T>> merged;
    for (const Range<T>& range : ranges) {
        if (range.begin > range.end) {
            throw std::invalid_argument("range is reversed");
        }

        if (merged.empty()) {
            merged.push_back(range);
            continue;
        }

        Range<T>& previous = merged.back();
        const bool overlaps = range.begin <= previous.end;
        // соседние диапазоны можно склеить: 1-3 и 4-5 эквивалентны 1-5
        const bool adjacent =
            previous.end != std::numeric_limits<T>::max() &&
            range.begin == previous.end + 1;

        if (overlaps || adjacent) {
            previous.end = std::max(previous.end, range.end);
        }
        else {
            merged.push_back(range);
        }
    }

    return merged;
}

// для оптимизации поиска включений в ranges
template <typename T>
class RangeFilter {
public:
    void setRanges(std::vector<Range<T>> ranges) {
        ranges_ = normalizeRanges(std::move(ranges));
    }

    bool contains(T value) const noexcept {
        if (ranges_.empty()) {
            return true;
        }

        // для адресов порядок обращений не гарантирован, поэтому использую обычный бинарный поиск
        const auto it = std::upper_bound(
            ranges_.begin(),
            ranges_.end(),
            value,
            [](T lhs, const Range<T>& rhs) {
                return lhs < rhs.begin;
            }
        );

        if (it == ranges_.begin()) {
            return false;
        }

        const auto candidate = std::prev(it);
        return value <= candidate->end;
    }

    bool intersects(T begin, T end) const noexcept {
        if (ranges_.empty()) {
            return true;
        }

        if (begin > end) {
            return false;
        }

        // ищем последний диапазон, который начинается не позже конца проверяемого диапазона
        const auto it = std::upper_bound(
            ranges_.begin(),
            ranges_.end(),
            end,
            [](T lhs, const Range<T>& rhs) {
                return lhs < rhs.begin;
            }
        );

        if (it == ranges_.begin()) {
            return false;
        }

        const auto candidate = std::prev(it);
        return begin <= candidate->end;
    }

    void reset() noexcept {}

private:
    std::vector<Range<T>> ranges_;
};

using TickRangeFilter = RangeFilter<std::uint64_t>;
using AddressRangeFilter = RangeFilter<std::uint32_t>;

std::vector<TickRange> parseTickRanges(std::string_view text);
std::vector<AddressRange> parseAddressRanges(std::string_view text);

}
