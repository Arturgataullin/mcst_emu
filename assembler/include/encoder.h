#pragma once

#include <cstdint>
#include <vector>

#include "parser.h"

namespace assembler {

class Encoder {
public:
    std::vector<std::uint8_t> encode(const Program& program) const;

private:
    std::uint32_t encodeInstruction(const Instruction& instruction) const;

    std::uint8_t requireRegister(const Operand& operand, const SourceLocation& location, const char* operandName) const;
    std::uint16_t requireImmediate(const Operand& operand, const SourceLocation& location, const char* operandName) const;
    std::uint8_t requireImmediate8(const Operand& operand, const SourceLocation& location, const char* operandName) const;

    [[noreturn]] void fail(const SourceLocation& location, const std::string& message) const;
};

}
