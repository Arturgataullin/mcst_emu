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
    std::uint16_t value = 0;
};

using Operand = std::variant<RegisterOperand, ImmediateOperand>;

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

    Program parseProgram();

private:
    bool isAtEnd() const noexcept;
    const Token& current() const noexcept;
    const Token& previous() const noexcept;
    const Token& advance() noexcept;
    bool check(TokenType type) const noexcept;
    bool match(TokenType type) noexcept;

    void skipNewLines() noexcept;

    Instruction parseInstruction();
    common::Operation parseOperation(const Token& token) const;
    RegisterOperand parseRegisterOperand();
    ImmediateOperand parseImmediateOperand();

    void expect(TokenType type, const std::string& message);

    void fail(const SourceLocation& location, const std::string& message) const;

private:
    std::vector<Token> tokens_;
    std::size_t pos_ = 0;
};


}