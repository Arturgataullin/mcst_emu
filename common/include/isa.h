#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string_view>

// all operands store in register memory
// op memory is only for commands

namespace common {

    inline constexpr std::size_t registerCount = 16;
    inline constexpr std::size_t registerSize = 32;
    inline constexpr std::size_t regMemoreSize = registerCount * registerSize; // 16 registers of 4 bytes each

    
    inline constexpr std::size_t opMemorySize = 1024; // for 32 instructions of 4 bytes each
    inline constexpr std::uint32_t resetAddress = 0;
    
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
