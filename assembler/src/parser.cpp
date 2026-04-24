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
        const Operation op = parseOperation(opToken);

        Instruction instruction;
        instruction.operation = op;
        instruction.location = opToken.location;

        switch (op) {
            case Operation::LI:
                instruction.operands.push_back(parseRegisterOperand());
                instruction.operands.push_back(parseImmediateOperand());
                break;

            case Operation::ADD:
                instruction.operands.push_back(parseRegisterOperand());
                instruction.operands.push_back(parseRegisterOperand());
                instruction.operands.push_back(parseRegisterOperand());
                break;
        }

        expectEndOfLineOrFile();
        return instruction;
    }

    Operation Parser::parseOperation(const Token& token) const {
        if (token.lexeme == "LI") {
            return Operation::LI;
        }
        if (token.lexeme == "ADD") {
            return Operation::ADD;
        }

        fail(token.location, "unsupported operation '" + token.lexeme + "'");
    }

    RegisterOperand Parser::parseRegisterOperand() {
        expect(TokenType::Register, "expected register operand");

        const Token token = advance();
        return RegisterOperand{token.lexeme};
    }

    ImmediateOperand Parser::parseImmediateOperand() {
        expect(TokenType::Number, "expected immediate numeric operand");

        const Token token = advance();
        if (!token.numberValue.has_value()) {
            fail(token.location, "number token does not contain parsed value");
        }

        return ImmediateOperand{
            .value = static_cast<std::uint64_t>(*token.numberValue)
        };
    }

    void Parser::expect(TokenType type, const std::string& message) {
        if (!check(type)) {
            fail(current().location, message + ", got " + std::string(toString(current().type)));
        }
    }

    void Parser::expectEndOfLineOrFile() {
        if (check(TokenType::NewLine) || check(TokenType::EndOfFile)) {
            return;
        }

        fail(current().location, "expected end of line after instruction");
    }

    void Parser::fail(const SourceLocation& location, const std::string& message) const {
        throw std::runtime_error(formatErrorMessage(location, message));
    }

}

