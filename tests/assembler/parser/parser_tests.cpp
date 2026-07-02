#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "parser.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

using Catch::Matchers::ContainsSubstring;

namespace {

using assembler::ImmediateOperand;
using assembler::Instruction;
using assembler::Operand;
using common::Operation;
using assembler::Parser;
using assembler::Program;
using assembler::RegisterOperand;
using assembler::SourceLocation;
using assembler::Token;
using assembler::TokenType;

Token makeToken(TokenType type, std::string lexeme, std::size_t line, std::size_t column, 
    std::optional<std::int64_t> numberValue = std::nullopt) {
    return Token{
        .type = type,
        .lexeme = std::move(lexeme),
        .location = SourceLocation{line, column},
        .numberValue = numberValue
    };
}

Token op(std::string lexeme, std::size_t line = 1, std::size_t column = 1) {
    return makeToken(TokenType::Operation, std::move(lexeme), line, column);
}

Token reg(std::string lexeme, std::int64_t index, std::size_t line = 1, std::size_t column = 1) {
    return makeToken(TokenType::Register, std::move(lexeme), line, column, index);
}

Token num(std::string lexeme, std::int64_t value, std::size_t line = 1, std::size_t column = 1) {
    return makeToken(TokenType::Number, std::move(lexeme), line, column, value);
}

Token nl(std::size_t line = 1, std::size_t column = 1) {
    return makeToken(TokenType::NewLine, "", line, column);
}

Token eof(std::size_t line = 1, std::size_t column = 1) {
    return makeToken(TokenType::EndOfFile, "", line, column);
}

const RegisterOperand& asRegister(const Operand& operand) {
    return std::get<RegisterOperand>(operand);
}

const ImmediateOperand& asImmediate(const Operand& operand) {
    return std::get<ImmediateOperand>(operand);
}

}

TEST_CASE("parser parses single LI instruction") {
    std::vector<Token> tokens = {
        op("LI"),
        reg("R1", 1, 1, 4),
        num("0x10", 0x10, 1, 7),
        eof(1, 11)
    };

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    REQUIRE(program.instructions.size() == 1);

    const Instruction& inst = program.instructions[0];
    REQUIRE(inst.operation == common::Operation::LI);
    REQUIRE(inst.location.line == 1);
    REQUIRE(inst.location.column == 1);
    REQUIRE(inst.operands.size() == 2);

    REQUIRE(asRegister(inst.operands[0]).number == 1);
    REQUIRE(asImmediate(inst.operands[1]).value == 0x10);
}

TEST_CASE("parser parses single ADD instruction") {
    std::vector<Token> tokens = {
        op("ADD"),
        reg("R1", 1, 1, 5),
        reg("R2", 2, 1, 8),
        reg("R3", 3, 1, 11),
        eof(1, 13)
    };

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    REQUIRE(program.instructions.size() == 1);

    const Instruction& inst = program.instructions[0];
    REQUIRE(inst.operation == common::Operation::ADD);
    REQUIRE(inst.operands.size() == 3);

    REQUIRE(asRegister(inst.operands[0]).number == 1);
    REQUIRE(asRegister(inst.operands[1]).number == 2);
    REQUIRE(asRegister(inst.operands[2]).number == 3);
}

TEST_CASE("parser parses single LUI instruction") {
    std::vector<Token> tokens = {
        op("LUI"),
        reg("R1", 1, 1, 5),
        num("0x1234", 0x1234, 1, 8),
        eof(1, 14)
    };

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    REQUIRE(program.instructions.size() == 1);

    const Instruction& inst = program.instructions[0];
    REQUIRE(inst.operation == common::Operation::LUI);
    REQUIRE(inst.operands.size() == 2);

    REQUIRE(asRegister(inst.operands[0]).number == 1);
    REQUIRE(asImmediate(inst.operands[1]).value == 0x1234);
}

TEST_CASE("parser parses single SUB instruction") {
    std::vector<Token> tokens = {
        op("SUB"),
        reg("R1", 1, 1, 5),
        reg("R2", 2, 1, 8),
        reg("R3", 3, 1, 11),
        eof(1, 13)
    };

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    REQUIRE(program.instructions.size() == 1);

    const Instruction& inst = program.instructions[0];
    REQUIRE(inst.operation == common::Operation::SUB);
    REQUIRE(inst.operands.size() == 3);

    REQUIRE(asRegister(inst.operands[0]).number == 1);
    REQUIRE(asRegister(inst.operands[1]).number == 2);
    REQUIRE(asRegister(inst.operands[2]).number == 3);
}

TEST_CASE("parser parses single AND instruction") {
    std::vector<Token> tokens = {
        op("AND"),
        reg("R1", 1, 1, 5),
        reg("R2", 2, 1, 8),
        reg("R3", 3, 1, 11),
        eof(1, 13)
    };

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    REQUIRE(program.instructions.size() == 1);

    const Instruction& inst = program.instructions[0];
    REQUIRE(inst.operation == common::Operation::AND);
    REQUIRE(inst.operands.size() == 3);

    REQUIRE(asRegister(inst.operands[0]).number == 1);
    REQUIRE(asRegister(inst.operands[1]).number == 2);
    REQUIRE(asRegister(inst.operands[2]).number == 3);
}

TEST_CASE("parser parses single OR instruction") {
    std::vector<Token> tokens = {
        op("OR"),
        reg("R1", 1, 1, 4),
        reg("R2", 2, 1, 7),
        reg("R3", 3, 1, 10),
        eof(1, 12)
    };

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    REQUIRE(program.instructions.size() == 1);

    const Instruction& inst = program.instructions[0];
    REQUIRE(inst.operation == common::Operation::OR);
    REQUIRE(inst.operands.size() == 3);

    REQUIRE(asRegister(inst.operands[0]).number == 1);
    REQUIRE(asRegister(inst.operands[1]).number == 2);
    REQUIRE(asRegister(inst.operands[2]).number == 3);
}

TEST_CASE("parser parses single XOR instruction") {
    std::vector<Token> tokens = {
        op("XOR"),
        reg("R1", 1, 1, 5),
        reg("R2", 2, 1, 8),
        reg("R3", 3, 1, 11),
        eof(1, 13)
    };

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    REQUIRE(program.instructions.size() == 1);

    const Instruction& inst = program.instructions[0];
    REQUIRE(inst.operation == common::Operation::XOR);
    REQUIRE(inst.operands.size() == 3);

    REQUIRE(asRegister(inst.operands[0]).number == 1);
    REQUIRE(asRegister(inst.operands[1]).number == 2);
    REQUIRE(asRegister(inst.operands[2]).number == 3);
}

TEST_CASE("parser parses single SLL instruction") {
    std::vector<Token> tokens = {
        op("SLL"),
        reg("R1", 1, 1, 5),
        reg("R2", 2, 1, 8),
        reg("R3", 3, 1, 11),
        eof(1, 13)
    };

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    REQUIRE(program.instructions.size() == 1);

    const Instruction& inst = program.instructions[0];
    REQUIRE(inst.operation == common::Operation::SLL);
    REQUIRE(inst.operands.size() == 3);

    REQUIRE(asRegister(inst.operands[0]).number == 1);
    REQUIRE(asRegister(inst.operands[1]).number == 2);
    REQUIRE(asRegister(inst.operands[2]).number == 3);
}

TEST_CASE("parser parses single SRL instruction") {
    std::vector<Token> tokens = {
        op("SRL"),
        reg("R1", 1, 1, 5),
        reg("R2", 2, 1, 8),
        reg("R3", 3, 1, 11),
        eof(1, 13)
    };

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    REQUIRE(program.instructions.size() == 1);

    const Instruction& inst = program.instructions[0];
    REQUIRE(inst.operation == common::Operation::SRL);
    REQUIRE(inst.operands.size() == 3);

    REQUIRE(asRegister(inst.operands[0]).number == 1);
    REQUIRE(asRegister(inst.operands[1]).number == 2);
    REQUIRE(asRegister(inst.operands[2]).number == 3);
}

TEST_CASE("parser parses single SRA instruction") {
    std::vector<Token> tokens = {
        op("SRA"),
        reg("R1", 1, 1, 5),
        reg("R2", 2, 1, 8),
        reg("R3", 3, 1, 11),
        eof(1, 13)
    };

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    REQUIRE(program.instructions.size() == 1);

    const Instruction& inst = program.instructions[0];
    REQUIRE(inst.operation == common::Operation::SRA);
    REQUIRE(inst.operands.size() == 3);

    REQUIRE(asRegister(inst.operands[0]).number == 1);
    REQUIRE(asRegister(inst.operands[1]).number == 2);
    REQUIRE(asRegister(inst.operands[2]).number == 3);
}

TEST_CASE("parser parses single MUL instruction") {
    std::vector<Token> tokens = {
        op("MUL"),
        reg("R1", 1, 1, 5),
        reg("R2", 2, 1, 8),
        reg("R3", 3, 1, 11),
        eof(1, 13)
    };

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    REQUIRE(program.instructions.size() == 1);

    const Instruction& inst = program.instructions[0];
    REQUIRE(inst.operation == common::Operation::MUL);
    REQUIRE(inst.operands.size() == 3);

    REQUIRE(asRegister(inst.operands[0]).number == 1);
    REQUIRE(asRegister(inst.operands[1]).number == 2);
    REQUIRE(asRegister(inst.operands[2]).number == 3);
}

TEST_CASE("parser parses single UDIV instruction") {
    std::vector<Token> tokens = {
        op("UDIV"),
        reg("R1", 1, 1, 6),
        reg("R2", 2, 1, 9),
        reg("R3", 3, 1, 12),
        eof(1, 14)
    };

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    REQUIRE(program.instructions.size() == 1);

    const Instruction& inst = program.instructions[0];
    REQUIRE(inst.operation == common::Operation::UDIV);
    REQUIRE(inst.operands.size() == 3);

    REQUIRE(asRegister(inst.operands[0]).number == 1);
    REQUIRE(asRegister(inst.operands[1]).number == 2);
    REQUIRE(asRegister(inst.operands[2]).number == 3);
}

TEST_CASE("parser parses single SDIV instruction") {
    std::vector<Token> tokens = {
        op("SDIV"),
        reg("R1", 1, 1, 6),
        reg("R2", 2, 1, 9),
        reg("R3", 3, 1, 12),
        eof(1, 14)
    };

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    REQUIRE(program.instructions.size() == 1);

    const Instruction& inst = program.instructions[0];
    REQUIRE(inst.operation == common::Operation::SDIV);
    REQUIRE(inst.operands.size() == 3);

    REQUIRE(asRegister(inst.operands[0]).number == 1);
    REQUIRE(asRegister(inst.operands[1]).number == 2);
    REQUIRE(asRegister(inst.operands[2]).number == 3);
}

TEST_CASE("parser parses multiple instructions separated by newlines") {
    std::vector<Token> tokens = {
        nl(1, 1),
        op("LI", 2, 1),
        reg("R0", 0, 2, 4),
        num("0x1", 1, 2, 7),
        nl(2, 10),

        nl(3, 1),

        op("ADD", 4, 1),
        reg("R1", 1, 4, 5),
        reg("R1", 1, 4, 8),
        reg("R0", 0, 4, 11),
        nl(4, 13),

        eof(5, 1)
    };

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    REQUIRE(program.instructions.size() == 2);

    REQUIRE(program.instructions[0].operation == Operation::LI);
    REQUIRE(program.instructions[1].operation == Operation::ADD);

    REQUIRE(asRegister(program.instructions[0].operands[0]).number == 0);
    REQUIRE(asImmediate(program.instructions[0].operands[1]).value == 1);

    REQUIRE(asRegister(program.instructions[1].operands[0]).number == 1);
    REQUIRE(asRegister(program.instructions[1].operands[1]).number == 1);
    REQUIRE(asRegister(program.instructions[1].operands[2]).number == 0);
}

TEST_CASE("parser lowers MOV to OR") {
    std::vector<Token> tokens = {
        op("MOV"),
        reg("R1", 1, 1, 5),
        reg("R2", 2, 1, 8),
        eof(1, 10)
    };

    Program program = Parser(std::move(tokens)).parseProgram();

    REQUIRE(program.instructions.size() == 1);
    const Instruction& inst = program.instructions[0];
    REQUIRE(inst.operation == Operation::OR);
    REQUIRE(asRegister(inst.operands[0]).number == 1);
    REQUIRE(asRegister(inst.operands[1]).number == 2);
    REQUIRE(asRegister(inst.operands[2]).number == 2);
}

TEST_CASE("parser lowers NEG using assembler temporary register") {
    std::vector<Token> tokens = {
        op("NEG"),
        reg("R1", 1, 1, 5),
        reg("R2", 2, 1, 8),
        eof(1, 10)
    };

    Program program = Parser(std::move(tokens)).parseProgram();

    REQUIRE(program.instructions.size() == 2);
    REQUIRE(program.instructions[0].operation == Operation::LI);
    REQUIRE(asRegister(program.instructions[0].operands[0]).number == common::assemblerTempRegister);
    REQUIRE(asImmediate(program.instructions[0].operands[1]).value == 0);
    REQUIRE(program.instructions[1].operation == Operation::SUB);
    REQUIRE(asRegister(program.instructions[1].operands[0]).number == 1);
    REQUIRE(asRegister(program.instructions[1].operands[1]).number == common::assemblerTempRegister);
    REQUIRE(asRegister(program.instructions[1].operands[2]).number == 2);
}

TEST_CASE("parser lowers NOT using all-ones temporary value") {
    std::vector<Token> tokens = {
        op("NOT"),
        reg("R3", 3, 1, 5),
        eof(1, 7)
    };

    Program program = Parser(std::move(tokens)).parseProgram();

    REQUIRE(program.instructions.size() == 3);
    REQUIRE(program.instructions[0].operation == Operation::LI);
    REQUIRE(asImmediate(program.instructions[0].operands[1]).value == 0xFFFF);
    REQUIRE(program.instructions[1].operation == Operation::LUI);
    REQUIRE(asImmediate(program.instructions[1].operands[1]).value == 0xFFFF);
    REQUIRE(program.instructions[2].operation == Operation::XOR);
    REQUIRE(asRegister(program.instructions[2].operands[0]).number == 3);
    REQUIRE(asRegister(program.instructions[2].operands[1]).number == 3);
    REQUIRE(asRegister(program.instructions[2].operands[2]).number == common::assemblerTempRegister);
}

TEST_CASE("parser lowers LFI into lower and upper loads") {
    std::vector<Token> tokens = {
        op("LFI"),
        reg("R4", 4, 1, 5),
        num("0x12345678", 0x12345678, 1, 8),
        eof(1, 18)
    };

    Program program = Parser(std::move(tokens)).parseProgram();

    REQUIRE(program.instructions.size() == 2);
    REQUIRE(program.instructions[0].operation == Operation::LI);
    REQUIRE(asRegister(program.instructions[0].operands[0]).number == 4);
    REQUIRE(asImmediate(program.instructions[0].operands[1]).value == 0x5678);
    REQUIRE(program.instructions[1].operation == Operation::LUI);
    REQUIRE(asRegister(program.instructions[1].operands[0]).number == 4);
    REQUIRE(asImmediate(program.instructions[1].operands[1]).value == 0x1234);
}

TEST_CASE("parser lowers LFI into lower and upper loads and parses next instruction") {
    std::vector<Token> tokens = {
        op("LFI"),
        reg("R4", 4, 1, 5),
        num("0x12345678", 0x12345678, 1, 8),
        nl(1, 18),
        op("LI"),
        reg("R4", 4, 2, 4),
        num("0x1234", 0x1234, 2, 8),
        eof(2, 13)
    };

    Program program = Parser(std::move(tokens)).parseProgram();

    REQUIRE(program.instructions.size() == 3);
    REQUIRE(program.instructions[0].operation == Operation::LI);
    REQUIRE(asRegister(program.instructions[0].operands[0]).number == 4);
    REQUIRE(asImmediate(program.instructions[0].operands[1]).value == 0x5678);
    REQUIRE(program.instructions[1].operation == Operation::LUI);
    REQUIRE(asRegister(program.instructions[1].operands[0]).number == 4);
    REQUIRE(asImmediate(program.instructions[1].operands[1]).value == 0x1234);
    REQUIRE(program.instructions[2].operation == Operation::LI);
    REQUIRE(asRegister(program.instructions[2].operands[0]).number == 4);
    REQUIRE(asImmediate(program.instructions[2].operands[1]).value == 0x1234);
}

TEST_CASE("parser rejects instruction with missing operand") {
    std::vector<Token> tokens = {
        op("LI"),
        reg("R1", 1, 1, 4),
        eof(1, 6)
    };

    Parser parser(std::move(tokens));

    REQUIRE_THROWS_WITH(parser.parseProgram(), ContainsSubstring("expected immediate numeric operand"));
}

TEST_CASE("parser rejects ADD with too few operands") {
    std::vector<Token> tokens = {
        op("ADD"),
        reg("R1", 1, 1, 5),
        reg("R2", 2, 1, 8),
        eof(1, 10)
    };

    Parser parser(std::move(tokens));

    REQUIRE_THROWS_WITH(parser.parseProgram(), ContainsSubstring("expected register operand"));
}

TEST_CASE("parser rejects extra tokens after instruction") {
    std::vector<Token> tokens = {
        op("LI"),
        reg("R1", 1, 1, 4),
        num("0x2", 2, 1, 7),
        num("0x3", 3, 1, 11),
        eof(1, 14)
    };

    Parser parser(std::move(tokens));

    REQUIRE_THROWS_WITH(parser.parseProgram(), ContainsSubstring("expected end of line after instruction"));
}

TEST_CASE("parser rejects unsupported operation token") {
    std::vector<Token> tokens = {
        op("INVALID"),
        reg("R1", 1, 1, 5),
        reg("R2", 2, 1, 8),
        reg("R3", 3, 1, 11),
        eof(1, 13)
    };

    Parser parser(std::move(tokens));

    REQUIRE_THROWS_WITH(parser.parseProgram(), ContainsSubstring("unsupported operation 'INVALID'"));
}

TEST_CASE("parser rejects out-of-range register") {
    std::vector<Token> tokens = {
        op("LI"),
        reg("R28", 28, 1, 4),
        num("0x1", 1, 1, 8),
        eof(1, 11)
    };

    Parser parser(std::move(tokens));

    REQUIRE_THROWS_WITH(parser.parseProgram(), ContainsSubstring("register number must be from 0 to 15"));
}

TEST_CASE("parser rejects too large immediate") {
    std::vector<Token> tokens = {
        op("LI"),
        reg("R1", 1, 1, 4),
        num("0x10000", 0x10000, 1, 7),
        eof(1, 14)
    };

    Parser parser(std::move(tokens));

    REQUIRE_THROWS_WITH(parser.parseProgram(), ContainsSubstring("immediate value is out of range"));
}

TEST_CASE("parser rejects wrong operand type for LI destination") {
    std::vector<Token> tokens = {
        op("LI"),
        num("0x1", 1, 1, 4),
        num("0x2", 2, 1, 8),
        eof(1, 11)
    };

    Parser parser(std::move(tokens));

    REQUIRE_THROWS_WITH(parser.parseProgram(), ContainsSubstring("expected register operand"));
}

TEST_CASE("parser rejects wrong operand type for ADD source") {
    std::vector<Token> tokens = {
        op("ADD"),
        reg("R1", 1, 1, 5),
        num("0x2", 2, 1, 8),
        reg("R3", 3, 1, 12),
        eof(1, 14)
    };

    Parser parser(std::move(tokens));

    REQUIRE_THROWS_WITH(parser.parseProgram(), ContainsSubstring("expected register operand"));
}
