#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "lexer.h"
#include "isa.h"

namespace assembler {

struct RegisterOperand {
    std::uint8_t number = 0;
};

struct ImmediateOperand {
    std::uint32_t value = 0;
};

// отдельный тип не позволяет перепутать SCR с регистром общего назначения
struct StatusRegisterOperand {
    common::StatusRegister reg = common::StatusRegister::SpTop;
};

using Operand = std::variant<RegisterOperand, ImmediateOperand, StatusRegisterOperand>;

struct Instruction {
    common::Operation operation{};
    std::vector<Operand> operands;
    SourceLocation location{};
};

struct Program {
    std::vector<Instruction> instructions;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    [[nodiscard]] Program parseProgram();

private:
    [[nodiscard]] bool isAtEnd() const noexcept;
    [[nodiscard]] const Token& current() const noexcept;
    [[nodiscard]] const Token& previous() const noexcept;
    const Token& advance() noexcept;
    [[nodiscard]] bool check(TokenType type) const noexcept;
    bool match(TokenType type) noexcept;

    void skipNewLines() noexcept;

    [[nodiscard]] Instruction parseInstruction();
    
    void appendLoweredInstruction(Instruction instruction, std::vector<Instruction>& out);

    [[nodiscard]] RegisterOperand parseRegisterOperand();
    [[nodiscard]] ImmediateOperand parseImmediateOperand(std::uint64_t maxValue);
    [[nodiscard]] StatusRegisterOperand parseStatusRegisterOperand();

    void expect(TokenType type, const std::string& message);

    [[noreturn]] void fail(const SourceLocation& location, const std::string& message) const;

private:
    std::vector<Token> tokens_;
    std::size_t pos_ = 0;
};


}
