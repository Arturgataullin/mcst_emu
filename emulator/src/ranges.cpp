#include "ranges.h"

#include <charconv>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>

namespace emulator {
namespace {

// число сначала читается в uint64_t, чтобы отдельно проверить переполнение нужного типа
template <typename T>
T parseRangeValue(std::string_view text, std::string_view optionName) {
    if (text.empty()) {
        throw std::invalid_argument("empty value in " + std::string(optionName));
    }

    std::uint64_t value = 0;
    const char* first = text.data();
    const char* last = first + text.size();
    const auto result = std::from_chars(first, last, value);

    if (result.ec != std::errc{} || result.ptr != last) {
        throw std::invalid_argument(
            "invalid value in " + std::string(optionName) + ": " + std::string(text)
        );
    }

    if (value > std::numeric_limits<T>::max()) {
        throw std::invalid_argument(
            "value is too large in " + std::string(optionName) + ": " + std::string(text)
        );
    }

    return static_cast<T>(value);
}

// общий парсер синтаксиса a,b-c; смысл диапазона задает вызывающая функция
template <typename T>
std::vector<Range<T>> parseRanges(std::string_view text, std::string_view optionName) { // text - перечисление range через запятую
    if (text.empty()) {
        throw std::invalid_argument(std::string(optionName) + " requires a non-empty value");
    }

    std::vector<Range<T>> ranges;
    std::size_t position = 0;

    while (position <= text.size()) {
        const std::size_t comma = text.find(',', position);
        const std::size_t tokenEnd =
            comma == std::string_view::npos ? text.size() : comma;
        const std::string_view token = text.substr(position, tokenEnd - position);

        if (token.empty()) {
            throw std::invalid_argument("empty range in " + std::string(optionName));
        }

        const std::size_t dash = token.find('-');
        if (dash == std::string_view::npos) {
            const T value = parseRangeValue<T>(token, optionName);
            ranges.push_back({value, value});
        }
        else {
            if (token.find('-', dash + 1) != std::string_view::npos) {
                throw std::invalid_argument(
                    "invalid range in " + std::string(optionName) + ": " + std::string(token)
                );
            }

            const T begin = parseRangeValue<T>(token.substr(0, dash), optionName);
            const T end = parseRangeValue<T>(token.substr(dash + 1), optionName);
            if (begin > end) {
                throw std::invalid_argument(
                    "reversed range in " + std::string(optionName) + ": " + std::string(token)
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

std::vector<TickRange> parseTickRanges(std::string_view text) {
    return parseRanges<std::uint64_t>(text, "--trace-ticks");
}

std::vector<AddressRange> parseAddressRanges(std::string_view text) {
    return parseRanges<std::uint32_t>(text, "--trace-ram-addrs");
}

}
