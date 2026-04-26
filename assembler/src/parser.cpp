#include "parser.h"

#include <sstream>

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

Program Parser::parseProgram() {
    Program program;

    skipNewLines();

    while (!isAtEnd()) {
        program.instructions.push_back(parseInstruction());
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

    const auto op = common::operationFromSring(opToken.lexeme);
    if (!op.has_value()) {
        fail(opToken.location, "unsupported operation '" + opToken.lexeme + "'");
    }

    Instruction instruction;
    instruction.operation = *op;
    instruction.location = opToken.location;

    switch (*op) {
        case common::Operation::LI:
            instruction.operands.push_back(parseRegisterOperand());
            instruction.operands.push_back(parseImmediateOperand());
            break;

        case common::Operation::ADD:
            instruction.operands.push_back(parseRegisterOperand());
            instruction.operands.push_back(parseRegisterOperand());
            instruction.operands.push_back(parseRegisterOperand());
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

ImmediateOperand Parser::parseImmediateOperand() {
    expect(TokenType::Number, "expected immediate numeric operand");

    const Token token = advance();
    if (!token.numberValue.has_value()) {
        fail(token.location, "number token does not contain parsed value");
    }

    const auto val = *token.numberValue;

    if (val > common::immediateMax) {
        fail(token.location, "immediate value must be in range [0x0, " + std::to_string(common::immediateMax) + "]");
    }

    return ImmediateOperand{
        .value = static_cast<std::uint16_t>(val)
    };
}

void Parser::expect(TokenType type, const std::string& message) {
    if (!check(type)) {
        fail(current().location, message + ", got " + std::string(toString(current().type)));
    }
}

void Parser::fail(const SourceLocation& location, const std::string& message) const {
    throw std::runtime_error(formatErrorMessage(location, message));
}

}

