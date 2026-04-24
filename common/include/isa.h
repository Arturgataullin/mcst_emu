#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace common {
    inline constexpr std::size_t registerCount = 16;
    inline constexpr std::uint64_t immediateMax = 0xFFFF;   

    enum class Operation {
        LI,
        ADD
    };

    std::optional<Operation> operationFromSring(std::string_view lexeme);
    std::string_view toString(Operation op);

}
