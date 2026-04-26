#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace common {
    inline constexpr size_t registerCount = 16;
    inline constexpr uint64_t immediateMax = 0xFFFF;   
    inline constexpr size_t instructionSizeBytes = 4;

    enum class Operation {
        LI,
        ADD
    };

    enum class Opcode : uint8_t {
        LI = 0x00,
        ADD = 0x02
    };

    std::optional<Operation> operationFromSring(std::string_view lexeme);
    std::string_view toString(Operation op);
    Opcode opcodeForOperation(Operation op);

}
