#include "parser.h"

#include <sstream>
#include <string>
#include <utility>

namespace assembler {

namespace {
    std::string formatErrorMessage(const SourceLocation& location, const std::string& message) {
        std::ostringstream out;
        out << "parser error at line " << location.line
            << ", column " << location.column
            << ": " << message;
        return out.str();
    }
}

Parser::Parser(std::vector<Token> tokens)
    : tokens_(std::move(tokens)) {}

namespace {

RegisterOperand makeRegisterOperand(std::uint8_t reg) {
    return RegisterOperand{
        .number = reg
    };
}

ImmediateOperand makeImmediateOperand(std::uint16_t value) {
    return ImmediateOperand{
        .value = value
    };
}

Instruction makeLoadInstruction(
    common::Operation operation,
    std::uint8_t reg,
    std::uint16_t value,
    SourceLocation loc
) {
    if (operation != common::Operation::LI &&
        operation != common::Operation::LUI) {
        throw std::logic_error("makeLoadInstruction expects LI or LUI");
    }

    Instruction instruction;
    instruction.operation = operation;
    instruction.location = loc;

    instruction.operands.reserve(2);
    instruction.operands.push_back(makeRegisterOperand(reg));
    instruction.operands.push_back(makeImmediateOperand(value));

    return instruction;
}

Instruction makeRegisterInstruction(
    common::Operation operation,
    std::uint8_t destination,
    std::uint8_t leftSource,
    std::uint8_t rightSource,
    SourceLocation location
) {
    Instruction instruction;
    instruction.operation = operation;
    instruction.location = location;

    instruction.operands.reserve(3);
    instruction.operands.push_back(makeRegisterOperand(destination));
    instruction.operands.push_back(makeRegisterOperand(leftSource));
    instruction.operands.push_back(makeRegisterOperand(rightSource));

    return instruction;
}

std::uint8_t getRegisterNumber(const Instruction& instruction, std::size_t index) {
    return std::get<RegisterOperand>(instruction.operands.at(index)).number;
}

std::uint32_t getImmediateValue(const Instruction& instruction, std::size_t index) {
    return std::get<ImmediateOperand>(instruction.operands.at(index)).value;
}

void appendMov(const Instruction& instruction, std::vector<Instruction>& out) {
    const auto destination = getRegisterNumber(instruction, 0);
    const auto source = getRegisterNumber(instruction, 1);

    out.push_back(makeRegisterInstruction(
        common::Operation::OR,
        destination,
        source,
        source,
        instruction.location
    ));
}

void appendNeg(const Instruction& instruction, std::vector<Instruction>& out) {
    const auto destination = getRegisterNumber(instruction, 0);
    const auto source = getRegisterNumber(instruction, 1);
    const auto temp = common::assemblerTempRegister;

    out.push_back(makeLoadInstruction(common::Operation::LI, temp, 0, instruction.location));
    out.push_back(makeRegisterInstruction(
        common::Operation::SUB,
        destination,
        temp,
        source,
        instruction.location
    ));
}

void appendNot(const Instruction& instruction, std::vector<Instruction>& out) {
    const auto reg = getRegisterNumber(instruction, 0);
    const auto temp = common::assemblerTempRegister;

    out.push_back(makeLoadInstruction(common::Operation::LI, temp, 0xFFFF, instruction.location));
    out.push_back(makeLoadInstruction(common::Operation::LUI, temp, 0xFFFF, instruction.location));
    out.push_back(makeRegisterInstruction(
        common::Operation::XOR,
        reg,
        reg,
        temp,
        instruction.location
    ));
}

void appendLfi(const Instruction& instruction, std::vector<Instruction>& out) {
    const auto destination = getRegisterNumber(instruction, 0);
    const auto value = getImmediateValue(instruction, 1);
    const auto lower = static_cast<std::uint16_t>(value & 0xFFFFu);
    const auto upper = static_cast<std::uint16_t>(value >> 16);

    out.push_back(makeLoadInstruction(common::Operation::LI, destination, lower, instruction.location));
    out.push_back(makeLoadInstruction(common::Operation::LUI, destination, upper, instruction.location));
}

void appendRet(const Instruction& instruction, std::vector<Instruction>& out) {
    Instruction lowered;
    lowered.operation = common::Operation::AJMP;
    lowered.location = instruction.location;
    lowered.operands.push_back(RegisterOperand{common::returnAddressRegister});
    out.push_back(std::move(lowered));
}

}

void Parser::appendLoweredInstruction(Instruction instruction, std::vector<Instruction>& out) {
    switch (instruction.operation) {
        case common::Operation::MOV:
            appendMov(instruction, out);
            return;

        case common::Operation::NEG:
            appendNeg(instruction, out);
            return;

        case common::Operation::NOT:
            appendNot(instruction, out);
            return;

        case common::Operation::LFI:
            appendLfi(instruction, out);
            return;

        case common::Operation::RET:
            appendRet(instruction, out);
            return;

        default:
            out.push_back(instruction);
            return;
    }
}

Program Parser::parseProgram() {
    Program program;

    skipNewLines();

    while (!isAtEnd()) {
        appendLoweredInstruction(parseInstruction(), program.instructions);
        skipNewLines();
    }

    return program;
}

bool Parser::isAtEnd() const noexcept {
    return current().type == TokenType::EndOfFile;
}

const Token& Parser::current() const noexcept {
    return tokens_[pos_];
}

const Token& Parser::previous() const noexcept {
    return tokens_[pos_ - 1];
}

const Token& Parser::advance() noexcept {
    if (!isAtEnd()) {
        pos_++;
    }
    return previous();
}

bool Parser::check(TokenType type) const noexcept {
    if (isAtEnd()) {
        return type == TokenType::EndOfFile;
    }
    return current().type == type;
}

bool Parser::match(TokenType type) noexcept {
    if (!check(type)) {
        return false;
    }
    advance();
    return true;
}

void Parser::skipNewLines() noexcept {
    while (match(TokenType::NewLine)) {}
}

Instruction Parser::parseInstruction() {
    expect(TokenType::Operation, "expected operation at start of instruction");

    const Token opToken = advance();

    const auto op = common::operationFromString(opToken.lexeme);
    if (!op.has_value()) {
        fail(opToken.location, "unsupported operation '" + opToken.lexeme + "'");
    }

    Instruction instruction;
    instruction.operation = *op;
    instruction.location = opToken.location;

    switch (*op) {
        case common::Operation::LI:
        case common::Operation::LUI:
            instruction.operands.push_back(parseRegisterOperand());
            instruction.operands.push_back(parseImmediateOperand(common::immediate16Max));
            break;
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
        case common::Operation::SDIV:
        case common::Operation::EQ:
        case common::Operation::NE:
        case common::Operation::LT:
        case common::Operation::GE:
        case common::Operation::SLT:
        case common::Operation::SGE:
            instruction.operands.push_back(parseRegisterOperand());
            instruction.operands.push_back(parseRegisterOperand());
            instruction.operands.push_back(parseRegisterOperand());
            break;
        // новые команды используют формат opcode a b i: два регистра и 8-битный immediate
        case common::Operation::LDB:
        case common::Operation::LDH:
        case common::Operation::LDW:
        case common::Operation::STB:
        case common::Operation::STH:
        case common::Operation::STW:
        case common::Operation::SXT:
        case common::Operation::BSWAP:
            instruction.operands.push_back(parseRegisterOperand());
            instruction.operands.push_back(parseRegisterOperand());
            instruction.operands.push_back(parseImmediateOperand(common::immediate8Max));
            break;
        // в SCRW индекс SCR кодируется раньше регистра-источника, а в SCRR - после регистра-приемника
        case common::Operation::SCRW:
            instruction.operands.push_back(parseStatusRegisterOperand());
            instruction.operands.push_back(parseRegisterOperand());
            break;
        case common::Operation::SCRR:
            instruction.operands.push_back(parseRegisterOperand());
            instruction.operands.push_back(parseStatusRegisterOperand());
            break;
        // литерал ASPI пока хранится как беззнаковое 16-битное значение; знаковая интерпретация выполняется эмулятором
        case common::Operation::ASPI:
            instruction.operands.push_back(parseRegisterOperand());
            instruction.operands.push_back(parseImmediateOperand(common::immediate16Max));
            break;
        case common::Operation::ASPR:
            instruction.operands.push_back(parseRegisterOperand());
            instruction.operands.push_back(parseRegisterOperand());
            break;
        // переходы используют 16-битное смещение в словах инструкций, а знаковость учитывает emulator
        case common::Operation::RJMP:
            instruction.operands.push_back(parseImmediateOperand(common::immediate16Max));
            break;
        case common::Operation::BRZ:
        case common::Operation::BRNZ:
            instruction.operands.push_back(parseRegisterOperand());
            instruction.operands.push_back(parseImmediateOperand(common::immediate16Max));
            break;
        case common::Operation::AJMP:
        case common::Operation::CALL:
            instruction.operands.push_back(parseRegisterOperand());
            break;
        case common::Operation::MOV:
        case common::Operation::NEG:
            instruction.operands.push_back(parseRegisterOperand());
            instruction.operands.push_back(parseRegisterOperand());
            break;
        case common::Operation::NOT:
            instruction.operands.push_back(parseRegisterOperand());
            break;
        case common::Operation::LFI:
            instruction.operands.push_back(parseRegisterOperand());
            instruction.operands.push_back(parseImmediateOperand(common::immediate32Max));
            break;
        case common::Operation::RET:
            break;
    }

    if (!check(TokenType::NewLine) && !check(TokenType::EndOfFile)) {
        fail(current().location, "expected end of line after instruction");
    }

    return instruction;
}

RegisterOperand Parser::parseRegisterOperand() {
    expect(TokenType::Register, "expected register operand");

    const Token token = advance();

    if (!token.numberValue.has_value()) {
        fail(token.location, "register token does not contain register index");
    }

    const auto reg = *token.numberValue;

    if (reg >= common::registerCount) {
        fail(token.location, "register number must be from 0 to " + std::to_string(common::registerCount - 1));
    }

    return RegisterOperand{static_cast<std::uint8_t>(reg)};
}

ImmediateOperand Parser::parseImmediateOperand(std::uint64_t maxValue) {
    expect(TokenType::Number, "expected immediate numeric operand");

    const Token token = advance();
    if (!token.numberValue.has_value()) {
        fail(token.location, "number token does not contain parsed value");
    }

    const auto val = *token.numberValue;
    if (val > maxValue) {
        fail(token.location, "immediate value is out of range");
    }

    return ImmediateOperand{
        .value = static_cast<std::uint32_t>(val)
    };
    
}

StatusRegisterOperand Parser::parseStatusRegisterOperand() {
    expect(TokenType::StatusRegister, "expected status register operand");

    const Token token = advance();
    if (!token.numberValue.has_value()) {
        fail(token.location, "status register token does not contain register index");
    }

    // lexer сохраняет индекс в numberValue, но parser повторно проверяет допустимый набор SCR
    const auto index = *token.numberValue;
    if (index == common::statusRegisterIndex(common::StatusRegister::Ip)) {
        return StatusRegisterOperand{common::StatusRegister::Ip};
    }
    if (index == common::statusRegisterIndex(common::StatusRegister::SpTop)) {
        return StatusRegisterOperand{common::StatusRegister::SpTop};
    }
    if (index == common::statusRegisterIndex(common::StatusRegister::SpSize)) {
        return StatusRegisterOperand{common::StatusRegister::SpSize};
    }
    fail(token.location, "status register index must be 0, 1 or 2");
}

void Parser::expect(TokenType type, const std::string& message) {
    if (!check(type)) {
        fail(current().location, message + ", got " + std::string(toString(current().type)));
    }
}

[[noreturn]] void Parser::fail(const SourceLocation& location, const std::string& message) const {
    throw std::runtime_error(formatErrorMessage(location, message));
}

}
