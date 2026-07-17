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
using assembler::StatusRegisterOperand;
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

Token scr(
    std::string lexeme,
    common::StatusRegister statusRegister,
    std::size_t line = 1,
    std::size_t column = 1
) {
    return makeToken(
        TokenType::StatusRegister,
        std::move(lexeme),
        line,
        column,
        common::statusRegisterIndex(statusRegister)
    );
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

const StatusRegisterOperand& asStatusRegister(const Operand& operand) {
    return std::get<StatusRegisterOperand>(operand);
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

TEST_CASE("parser parses memory-format instruction with two registers and imm8") {
    std::vector<Token> tokens = {
        op("LDB"),
        reg("R1", 1, 1, 5),
        reg("R2", 2, 1, 8),
        num("0xFF", 0xFF, 1, 11),
        eof(1, 15)
    };

    Parser parser(std::move(tokens));
    Program program = parser.parseProgram();

    REQUIRE(program.instructions.size() == 1);

    const Instruction& inst = program.instructions[0];
    REQUIRE(inst.operation == common::Operation::LDB);
    REQUIRE(inst.operands.size() == 3);

    REQUIRE(asRegister(inst.operands[0]).number == 1);
    REQUIRE(asRegister(inst.operands[1]).number == 2);
    REQUIRE(asImmediate(inst.operands[2]).value == 0xFF);
}

TEST_CASE("parser parses control flow instructions") {
    std::vector<Token> tokens = {
        op("RJMP", 1, 1),
        num("0xfffe", 0xfffe, 1, 6),
        nl(1, 12),
        op("BRZ", 2, 1),
        reg("R1", 1, 2, 5),
        num("0x2", 2, 2, 8),
        nl(2, 11),
        op("BRNZ", 3, 1),
        reg("R2", 2, 3, 6),
        num("0xffff", 0xffff, 3, 9),
        nl(3, 15),
        op("AJMP", 4, 1),
        reg("R3", 3, 4, 6),
        nl(4, 8),
        op("CALL", 5, 1),
        reg("R4", 4, 5, 6),
        eof(5, 8)
    };

    Program program = Parser(std::move(tokens)).parseProgram();

    REQUIRE(program.instructions.size() == 5);

    REQUIRE(program.instructions[0].operation == Operation::RJMP);
    REQUIRE(program.instructions[0].operands.size() == 1);
    REQUIRE(asImmediate(program.instructions[0].operands[0]).value == 0xfffe);

    REQUIRE(program.instructions[1].operation == Operation::BRZ);
    REQUIRE(program.instructions[1].operands.size() == 2);
    REQUIRE(asRegister(program.instructions[1].operands[0]).number == 1);
    REQUIRE(asImmediate(program.instructions[1].operands[1]).value == 2);

    REQUIRE(program.instructions[2].operation == Operation::BRNZ);
    REQUIRE(program.instructions[2].operands.size() == 2);
    REQUIRE(asRegister(program.instructions[2].operands[0]).number == 2);
    REQUIRE(asImmediate(program.instructions[2].operands[1]).value == 0xffff);

    REQUIRE(program.instructions[3].operation == Operation::AJMP);
    REQUIRE(program.instructions[3].operands.size() == 1);
    REQUIRE(asRegister(program.instructions[3].operands[0]).number == 3);

    REQUIRE(program.instructions[4].operation == Operation::CALL);
    REQUIRE(program.instructions[4].operands.size() == 1);
    REQUIRE(asRegister(program.instructions[4].operands[0]).number == 4);
}

TEST_CASE("parser parses comparison instructions as register-register-register") {
    std::vector<Token> tokens = {
        op("EQ", 1, 1), reg("R1", 1, 1, 4), reg("R2", 2, 1, 7), reg("R3", 3, 1, 10), nl(1, 12),
        op("NE", 2, 1), reg("R4", 4, 2, 4), reg("R5", 5, 2, 7), reg("R6", 6, 2, 10), nl(2, 12),
        op("LT", 3, 1), reg("R7", 7, 3, 4), reg("R8", 8, 3, 7), reg("R9", 9, 3, 10), nl(3, 12),
        op("GE", 4, 1), reg("R10", 10, 4, 4), reg("R11", 11, 4, 8), reg("R12", 12, 4, 12), nl(4, 15),
        op("SLT", 5, 1), reg("R13", 13, 5, 5), reg("R14", 14, 5, 9), reg("R15", 15, 5, 13), nl(5, 16),
        op("SGE", 6, 1), reg("R0", 0, 6, 5), reg("R1", 1, 6, 8), reg("R2", 2, 6, 11), eof(6, 13)
    };

    Program program = Parser(std::move(tokens)).parseProgram();

    REQUIRE(program.instructions.size() == 6);
    REQUIRE(program.instructions[0].operation == Operation::EQ);
    REQUIRE(program.instructions[1].operation == Operation::NE);
    REQUIRE(program.instructions[2].operation == Operation::LT);
    REQUIRE(program.instructions[3].operation == Operation::GE);
    REQUIRE(program.instructions[4].operation == Operation::SLT);
    REQUIRE(program.instructions[5].operation == Operation::SGE);

    for (const Instruction& instruction : program.instructions) {
        REQUIRE(instruction.operands.size() == 3);
    }
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

TEST_CASE("parser lowers RET into jump through return address register") {
    std::vector<Token> tokens = {
        op("RET"),
        eof(1, 4)
    };

    Program program = Parser(std::move(tokens)).parseProgram();

    REQUIRE(program.instructions.size() == 1);
    REQUIRE(program.instructions[0].operation == Operation::AJMP);
    REQUIRE(program.instructions[0].operands.size() == 1);
    REQUIRE(asRegister(program.instructions[0].operands[0]).number == common::returnAddressRegister);
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

TEST_CASE("parser parses stack instructions") {
    std::vector<Token> tokens = {
        op("SCRW", 1, 1),
        scr("SP_TOP", common::StatusRegister::SpTop, 1, 6),
        reg("R5", 5, 1, 13),
        nl(1, 15),
        op("SCRR", 2, 1),
        reg("R6", 6, 2, 6),
        scr("SP_SIZE", common::StatusRegister::SpSize, 2, 9),
        nl(2, 16),
        op("ASPI", 3, 1),
        reg("R7", 7, 3, 6),
        num("0xfff0", 0xfff0, 3, 9),
        nl(3, 15),
        op("ASPR", 4, 1),
        reg("R8", 8, 4, 6),
        reg("R9", 9, 4, 9),
        eof(4, 11)
    };

    Program program = Parser(std::move(tokens)).parseProgram();

    REQUIRE(program.instructions.size() == 4);

    const Instruction& scrw = program.instructions[0];
    REQUIRE(scrw.operation == Operation::SCRW);
    REQUIRE(scrw.operands.size() == 2);
    CHECK(asStatusRegister(scrw.operands[0]).reg == common::StatusRegister::SpTop);
    CHECK(asRegister(scrw.operands[1]).number == 5);

    const Instruction& scrr = program.instructions[1];
    REQUIRE(scrr.operation == Operation::SCRR);
    REQUIRE(scrr.operands.size() == 2);
    CHECK(asRegister(scrr.operands[0]).number == 6);
    CHECK(asStatusRegister(scrr.operands[1]).reg == common::StatusRegister::SpSize);

    const Instruction& aspi = program.instructions[2];
    REQUIRE(aspi.operation == Operation::ASPI);
    REQUIRE(aspi.operands.size() == 2);
    CHECK(asRegister(aspi.operands[0]).number == 7);
    CHECK(asImmediate(aspi.operands[1]).value == 0xfff0);

    const Instruction& aspr = program.instructions[3];
    REQUIRE(aspr.operation == Operation::ASPR);
    REQUIRE(aspr.operands.size() == 2);
    CHECK(asRegister(aspr.operands[0]).number == 8);
    CHECK(asRegister(aspr.operands[1]).number == 9);
}

TEST_CASE("parser enforces stack instruction operand order") {
    SECTION("SCRW expects status register first") {
        Parser parser({
            op("SCRW"),
            reg("R5", 5, 1, 6),
            scr("SP_TOP", common::StatusRegister::SpTop, 1, 9),
            eof(1, 15)
        });

        REQUIRE_THROWS_WITH(
            parser.parseProgram(),
            ContainsSubstring("expected status register operand")
        );
    }

    SECTION("SCRR expects general-purpose register first") {
        Parser parser({
            op("SCRR"),
            scr("SP_TOP", common::StatusRegister::SpTop, 1, 6),
            reg("R5", 5, 1, 13),
            eof(1, 15)
        });

        REQUIRE_THROWS_WITH(
            parser.parseProgram(),
            ContainsSubstring("expected register operand")
        );
    }
}

TEST_CASE("parser rejects stack instruction with missing operand") {
    Parser parser({
        op("ASPR"),
        reg("R5", 5, 1, 6),
        eof(1, 8)
    });

    REQUIRE_THROWS_WITH(
        parser.parseProgram(),
        ContainsSubstring("expected register operand")
    );
}

TEST_CASE("parser rejects stack instruction with extra operand") {
    Parser parser({
        op("ASPI"),
        reg("R5", 5, 1, 6),
        num("0xfff0", 0xfff0, 1, 9),
        reg("R6", 6, 1, 16),
        eof(1, 18)
    });

    REQUIRE_THROWS_WITH(
        parser.parseProgram(),
        ContainsSubstring("expected end of line after instruction")
    );
}

TEST_CASE("parser rejects number instead of status register") {
    Parser parser({
        op("SCRW"),
        num("0x1", 1, 1, 6),
        reg("R5", 5, 1, 10),
        eof(1, 12)
    });

    REQUIRE_THROWS_WITH(
        parser.parseProgram(),
        ContainsSubstring("expected status register operand")
    );
}

TEST_CASE("parser rejects status register instead of general-purpose register") {
    Parser parser({
        op("ASPI"),
        scr("SP_TOP", common::StatusRegister::SpTop, 1, 6),
        num("0x1", 1, 1, 13),
        eof(1, 16)
    });

    REQUIRE_THROWS_WITH(
        parser.parseProgram(),
        ContainsSubstring("expected register operand")
    );
}

TEST_CASE("parser rejects ASPI immediate larger than 16 bits") {
    Parser parser({
        op("ASPI"),
        reg("R5", 5, 1, 6),
        num("0x10000", 0x10000, 1, 9),
        eof(1, 16)
    });

    REQUIRE_THROWS_WITH(
        parser.parseProgram(),
        ContainsSubstring("immediate value is out of range")
    );
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

TEST_CASE("parser rejects memory-format instruction with too large imm8") {
    std::vector<Token> tokens = {
        op("STW"),
        reg("R1", 1, 1, 5),
        reg("R2", 2, 1, 8),
        num("0x100", 0x100, 1, 11),
        eof(1, 16)
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
