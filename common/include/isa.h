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
    inline constexpr std::size_t regMemoreSize = registerCount * registerSize; // 16 registers of 32 bits each

    
    inline constexpr std::size_t opMemorySize = 64 * 1024;
    inline constexpr std::uint32_t resetAddress = 0;
    
    inline constexpr uint64_t immediate8Max = 0xFF;
    inline constexpr uint64_t immediate16Max = 0xFFFF;
    inline constexpr uint64_t immediate32Max = 0xFFFFFFFF;
    inline constexpr size_t instructionSizeBytes = 4;

    constexpr std::uint8_t assemblerTempRegister = 31;
    static_assert(assemblerTempRegister >= registerCount);

    // CALL сохраняет адрес возврата в этот регистр, а RET раскрывается в переход по нему
    constexpr std::uint8_t returnAddressRegister = 1;
    static_assert(returnAddressRegister < registerCount);

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
        LDB,
        LDH,
        LDW,
        STB,
        STH,
        STW,
        SXT,
        BSWAP,
        // команды доступа к SCR и изменения указателя стека
        SCRW,
        SCRR,
        ASPI,
        ASPR,
        // команды переходов
        RJMP,
        BRZ,
        BRNZ,
        AJMP,
        CALL,
        // команды сравнения
        EQ,
        NE,
        LT,
        GE,
        SLT,
        SGE,
        // pseudo commands
        MOV,
        NEG,
        NOT,
        LFI,
        RET
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
        SDIV = 0x0C,
        LDB = 0x10,
        LDH = 0x11,
        LDW = 0x12,
        STB = 0x13,
        STH = 0x14,
        STW = 0x15,
        SXT = 0x16,
        BSWAP = 0x17,
        // диапазон 0x20-0x23 выделен под управление SCR и стеком
        SCRW = 0x20,
        SCRR = 0x21,
        ASPI = 0x22,
        ASPR = 0x23,
        // диапазон 0x30-0x34 выделен под переходы
        RJMP = 0x30,
        BRZ = 0x31,
        BRNZ = 0x32,
        AJMP = 0x33,
        CALL = 0x34,
        // диапазон 0x40-0x45 выделен под сравнения
        EQ = 0x40,
        NE = 0x41,
        LT = 0x42,
        GE = 0x43,
        SLT = 0x44,
        SGE = 0x45
    };

    // индекс является частью машинной кодировки операнда SCR
    enum class StatusRegister : std::uint8_t {
        Ip = 0,
        SpTop = 1,
        SpSize = 2,
        // служебное значение задаёт размер массива SCR и не является регистром
        Count = 3
    };

    inline constexpr std::size_t statusRegisterCount = static_cast<std::size_t>(StatusRegister::Count);

    [[nodiscard]] std::optional<Operation> operationFromString(std::string_view lexeme);
    [[nodiscard]] std::optional<StatusRegister> statusRegisterFromString(std::string_view lexeme);
    [[nodiscard]] std::string_view toString(Operation op);
    [[nodiscard]] std::string_view toString(Opcode opcode);
    [[nodiscard]] std::string_view toString(StatusRegister reg);
    [[nodiscard]] Opcode opcodeForOperation(Operation op);

    [[nodiscard]] constexpr std::uint8_t statusRegisterIndex(StatusRegister reg) noexcept {
        return static_cast<std::uint8_t>(reg);
    }

}
