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
    
    inline constexpr uint64_t immediate16Max = 0xFFFF;
    inline constexpr uint64_t immediate32Max = 0xFFFFFFFF;
    inline constexpr size_t instructionSizeBytes = 4;

    constexpr std::uint8_t assemblerTempRegister = 31;
    static_assert(assemblerTempRegister >= registerCount);

    enum class Operation {
        LI,
        LUI,
        ADD,
        SUB,
        AND,
        OR,
        XOR,
        SLL,
        SRL,
        SRA,
        MUL,
        UDIV,
        SDIV,
        // pseudo commands
        MOV,
        NEG,
        NOT,
        LFI
    };

    enum class Opcode : uint8_t {
        LI = 0x00,
        LUI = 0x01,
        ADD = 0x02,
        SUB = 0x03,
        AND = 0x04,
        OR = 0x05,
        XOR = 0x06,
        SLL = 0x07,
        SRL = 0x08,
        SRA = 0x09,
        MUL = 0x0A,
        UDIV = 0x0B,
        SDIV = 0x0C
    };

    std::optional<Operation> operationFromString(std::string_view lexeme);
    std::string_view toString(Operation op);
    Opcode opcodeForOperation(Operation op);

}
