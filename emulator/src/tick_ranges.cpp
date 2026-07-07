#include "tick_ranges.h"

#include <algorithm>
#include <charconv>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace emulator {
namespace {

    std::uint64_t parseTick(std::string_view text) {
        if (text.empty()) {
            throw std::invalid_argument("empty tick in --trace-ticks");
        }

        std::uint64_t value = 0;
        const char* first = text.data();
        const char* last = first + text.size();
        const auto result = std::from_chars(first, last, value);

        if (result.ec != std::errc{} || result.ptr != last) {
            throw std::invalid_argument(
                "invalid tick in --trace-ticks: " + std::string(text)
            );
        }
        return value;
    }

    
    std::vector<TickRange> normalizeRanges(std::vector<TickRange> ranges) {
        // после сортировки пересечения можно объединить за один проход
        std::sort(ranges.begin(), ranges.end(), [](const TickRange& lhs, const TickRange& rhs) {
            if (lhs.begin != rhs.begin) {
                return lhs.begin < rhs.begin;
            }
            return lhs.end < rhs.end;
        });

        std::vector<TickRange> merged;
        for (const TickRange& range : ranges) {
            if (range.begin > range.end) {
                throw std::invalid_argument("trace tick range is reversed");
            }
            if (merged.empty()) {
                merged.push_back(range);
                continue;
            }

            TickRange& previous = merged.back();
            // соседние диапазоны эквивалентны одному непрерывному диапазону
            const bool adjacent =
                previous.end != std::numeric_limits<std::uint64_t>::max() && range.begin == previous.end + 1;

            if (range.begin <= previous.end || adjacent) {
                previous.end = std::max(previous.end, range.end);
            } 
            else {
                merged.push_back(range);
            }
        }
        return merged;
    }

}

void TickRangeFilter::setRanges(std::vector<TickRange> ranges) {
    currentRange_ = 0;
    ranges_ = normalizeRanges(std::move(ranges));
}

bool TickRangeFilter::contains(std::uint64_t tick) noexcept {
    if (ranges_.empty()) {
        return true;
    }
    for (std::size_t i = currentRange_; i < ranges_.size(); i++) {
        
        if (tick < ranges_[i].begin) {
            currentRange_ = i;
            return false;
        }
        if (tick <= ranges_[i].end) {
            currentRange_ = i;
            return true;
        }
    }
    return false;
}

void TickRangeFilter::reset() noexcept {
    currentRange_ = 0;
}

// функция больше не нормализует полученные диапазоны, теперь это делается в setRange
std::vector<TickRange> parseTickRanges(std::string_view text) {
    if (text.empty()) {
        throw std::invalid_argument("--trace-ticks requires a non-empty value");
    }

    std::vector<TickRange> ranges;
    std::size_t position = 0;

    while (position <= text.size()) {
        const std::size_t comma = text.find(',', position);
        const std::size_t tokenEnd =
            comma == std::string_view::npos ? text.size() : comma;
        const std::string_view token = text.substr(position, tokenEnd - position);

        if (token.empty()) {
            throw std::invalid_argument("empty range in --trace-ticks");
        }

        const std::size_t dash = token.find('-');
        if (dash == std::string_view::npos) {
            const std::uint64_t tick = parseTick(token);
            ranges.push_back({tick, tick});
        } 
        else {
            if (token.find('-', dash + 1) != std::string_view::npos) {
                throw std::invalid_argument(
                    "invalid range in --trace-ticks: " + std::string(token)
                );
            }

            const std::uint64_t begin = parseTick(token.substr(0, dash));
            const std::uint64_t end = parseTick(token.substr(dash + 1));
            if (begin > end) {
                throw std::invalid_argument(
                    "reversed range in --trace-ticks: " + std::string(token)
                );
            }
            ranges.push_back({begin, end});
        }

        if (comma == std::string_view::npos) {
            break;
        }
        position = comma + 1;
    }

    return ranges;
}


}