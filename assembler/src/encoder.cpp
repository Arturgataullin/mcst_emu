#include <sstream>
#include <stdexcept>

#include "encoder.h"
#include "isa.h"

namespace assembler {

namespace {

std::string formatErrorMessage(const SourceLocation& location, const std::string& message) {
    std::ostringstream out;
    out << "encoder error at line " << location.line
        << ", column " << location.column
        << ": " << message;
    return out.str();
}

}

std::vector<uint8_t> Encoder::encode(const Program& program) const {
    std::vector<uint8_t> bytes;
    bytes.reserve(program.instructions.size() * common::instructionSizeBytes);

    for (const auto& instruction : program.instructions) {
        const std::uint32_t word = encodeInstruction(instruction);

        bytes.push_back(static_cast<uint8_t>(word & 0xFF));
        bytes.push_back(static_cast<uint8_t>((word >> 8) & 0xFF));
        bytes.push_back(static_cast<uint8_t>((word >> 16) & 0xFF));
        bytes.push_back(static_cast<uint8_t>((word >> 24) & 0xFF));
    }

    return bytes;
}

std::uint32_t Encoder::encodeInstruction(const Instruction& instruction) const {
    switch (instruction.operation) {
        case common::Operation::LI:
        case common::Operation::LUI: {
            if (instruction.operands.size() != 2) {
                fail(instruction.location, std::string(common::toString(instruction.operation)) + " expects 2 operands");
            }

            const std::uint8_t rd = requireRegister(instruction.operands[0], instruction.location, "destination register");
            const std::uint16_t imm = requireImmediate(instruction.operands[1], instruction.location, "immediate");

            // byte0 = opcode
            // byte1 = rd
            // byte2 = imm low byte
            // byte3 = imm high byte
            std::uint32_t word = 0;
            word |= static_cast<std::uint32_t>(common::opcodeForOperation(instruction.operation));
            word |= static_cast<std::uint32_t>(rd) << 8;
            word |= static_cast<std::uint32_t>(imm & 0x00FF) << 16;
            word |= static_cast<std::uint32_t>((imm >> 8) & 0x00FF) << 24;
            return word;
        }

        case common::Operation::ADD:
        case common::Operation::SUB:
        case common::Operation::AND:
        case common::Operation::OR:
        case common::Operation::XOR:
        case common::Operation::SLL:
        case common::Operation::SRL:
        case common::Operation::SRA:
        case common::Operation::MUL:
        case common::Operation::UDIV:
        case common::Operation::SDIV: {
            if (instruction.operands.size() != 3) {
                fail(instruction.location, std::string(common::toString(instruction.operation)) + " expects 3 operands");
            }

            const uint8_t rd = requireRegister(instruction.operands[0], instruction.location, "destination register");
            const uint8_t rs = requireRegister(instruction.operands[1], instruction.location, "left source register");
            const uint8_t rt = requireRegister(instruction.operands[2], instruction.location, "right source register");

            // byte0 = opcode
            // byte1 = rd
            // byte2 = rs
            // byte3 = rt
            std::uint32_t word = 0;
            word |= static_cast<std::uint32_t>(common::opcodeForOperation(instruction.operation));
            word |= static_cast<std::uint32_t>(rd) << 8;
            word |= static_cast<std::uint32_t>(rs) << 16;
            word |= static_cast<std::uint32_t>(rt) << 24;
            return word;
        }

        case common::Operation::LDB:
        case common::Operation::LDH:
        case common::Operation::LDW:
        case common::Operation::STB:
        case common::Operation::STH:
        case common::Operation::STW:
        case common::Operation::SXT:
        case common::Operation::BSWAP: {
            if (instruction.operands.size() != 3) {
                fail(instruction.location, std::string(common::toString(instruction.operation)) + " expects 3 operands");
            }

            const std::uint8_t ra = requireRegister(instruction.operands[0], instruction.location, "first register");
            const std::uint8_t rb = requireRegister(instruction.operands[1], instruction.location, "second register");
            const std::uint8_t imm = requireImmediate8(instruction.operands[2], instruction.location, "immediate");

            // byte0 = opcode
            // byte1 = ra
            // byte2 = rb
            // byte3 = imm8
            std::uint32_t word = 0;
            word |= static_cast<std::uint32_t>(common::opcodeForOperation(instruction.operation));
            word |= static_cast<std::uint32_t>(ra) << 8;
            word |= static_cast<std::uint32_t>(rb) << 16;
            word |= static_cast<std::uint32_t>(imm) << 24;
            return word;
        }
        default:
            fail(instruction.location, "unknown operation during encoding");
    }
}

std::uint8_t Encoder::requireRegister(const Operand& operand, const SourceLocation& location, const char* operandName) const {
    if (!std::holds_alternative<RegisterOperand>(operand)) {
        fail(location, std::string(operandName) + " must be a register");
    }

    return std::get<RegisterOperand>(operand).number;
}

std::uint16_t Encoder::requireImmediate(const Operand& operand, const SourceLocation& location, const char* operandName) const {
    if (!std::holds_alternative<ImmediateOperand>(operand)) {
        fail(location, std::string(operandName) + " must be an immediate");
    }

    const auto value = std::get<ImmediateOperand>(operand).value;
    if (value > common::immediate16Max) {
        fail(location, std::string(operandName) + " must fit into 16 bits");
    }

    return static_cast<std::uint16_t>(value);
}

std::uint8_t Encoder::requireImmediate8(const Operand& operand, const SourceLocation& location, const char* operandName) const {
    if (!std::holds_alternative<ImmediateOperand>(operand)) {
        fail(location, std::string(operandName) + " must be an immediate");
    }

    const auto value = std::get<ImmediateOperand>(operand).value;
    if (value > common::immediate8Max) {
        fail(location, std::string(operandName) + " must fit into 8 bits");
    }

    return static_cast<std::uint8_t>(value);
}

[[noreturn]] void Encoder::fail(const SourceLocation& location, const std::string& message) const {
    throw std::runtime_error(formatErrorMessage(location, message));
}

}
