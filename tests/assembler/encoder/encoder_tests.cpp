#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "encoder.h"

#include <cstdint>
#include <utility>
#include <vector>

using Catch::Matchers::ContainsSubstring;

namespace {

using assembler::Encoder;
using assembler::ImmediateOperand;
using assembler::Instruction;
using assembler::Operand;
using assembler::Program;
using assembler::RegisterOperand;
using assembler::SourceLocation;
using assembler::StatusRegisterOperand;

Instruction makeLi(std::uint8_t rd, std::uint16_t imm, std::size_t line = 1, std::size_t column = 1) {
    Instruction inst;
    inst.operation = common::Operation::LI;
    inst.location = SourceLocation{line, column};
    inst.operands.push_back(RegisterOperand{rd});
    inst.operands.push_back(ImmediateOperand{imm});
    return inst;
}

Instruction makeLui(std::uint8_t rd, std::uint16_t imm, std::size_t line = 1, std::size_t column = 1) {
    Instruction inst;
    inst.operation = common::Operation::LUI;
    inst.location = SourceLocation{line, column};
    inst.operands.push_back(RegisterOperand{rd});
    inst.operands.push_back(ImmediateOperand{imm});
    return inst;
}

Instruction makeAdd(std::uint8_t rd, std::uint8_t rs, std::uint8_t rt, std::size_t line = 1, std::size_t column = 1) {
    Instruction inst;
    inst.operation = common::Operation::ADD;
    inst.location = SourceLocation{line, column};
    inst.operands.push_back(RegisterOperand{rd});
    inst.operands.push_back(RegisterOperand{rs});
    inst.operands.push_back(RegisterOperand{rt});
    return inst;
}

Instruction makeSub(std::uint8_t rd, std::uint8_t rs, std::uint8_t rt, std::size_t line = 1, std::size_t column = 1) {
    Instruction inst;
    inst.operation = common::Operation::SUB;
    inst.location = SourceLocation{line, column};
    inst.operands.push_back(RegisterOperand{rd});
    inst.operands.push_back(RegisterOperand{rs});
    inst.operands.push_back(RegisterOperand{rt});
    return inst;
}

Instruction makeAnd(std::uint8_t rd, std::uint8_t rs, std::uint8_t rt, std::size_t line = 1, std::size_t column = 1) {
    Instruction inst;
    inst.operation = common::Operation::AND;
    inst.location = SourceLocation{line, column};
    inst.operands.push_back(RegisterOperand{rd});
    inst.operands.push_back(RegisterOperand{rs});
    inst.operands.push_back(RegisterOperand{rt});
    return inst;
}

Instruction makeOr(std::uint8_t rd, std::uint8_t rs, std::uint8_t rt, std::size_t line = 1, std::size_t column = 1) {
    Instruction inst;
    inst.operation = common::Operation::OR;
    inst.location = SourceLocation{line, column};
    inst.operands.push_back(RegisterOperand{rd});
    inst.operands.push_back(RegisterOperand{rs});
    inst.operands.push_back(RegisterOperand{rt});
    return inst;
}

Instruction makeXor(std::uint8_t rd, std::uint8_t rs, std::uint8_t rt, std::size_t line = 1, std::size_t column = 1) {
    Instruction inst;
    inst.operation = common::Operation::XOR;
    inst.location = SourceLocation{line, column};
    inst.operands.push_back(RegisterOperand{rd});
    inst.operands.push_back(RegisterOperand{rs});
    inst.operands.push_back(RegisterOperand{rt});
    return inst;
}

Instruction makeSll(std::uint8_t rd, std::uint8_t rs, std::uint8_t rt, std::size_t line = 1, std::size_t column = 1) {
    Instruction inst;
    inst.operation = common::Operation::SLL;
    inst.location = SourceLocation{line, column};
    inst.operands.push_back(RegisterOperand{rd});
    inst.operands.push_back(RegisterOperand{rs});
    inst.operands.push_back(RegisterOperand{rt});
    return inst;
}

Instruction makeSrl(std::uint8_t rd, std::uint8_t rs, std::uint8_t rt, std::size_t line = 1, std::size_t column = 1) {
    Instruction inst;
    inst.operation = common::Operation::SRL;
    inst.location = SourceLocation{line, column};
    inst.operands.push_back(RegisterOperand{rd});
    inst.operands.push_back(RegisterOperand{rs});
    inst.operands.push_back(RegisterOperand{rt});
    return inst;
}

Instruction makeSra(std::uint8_t rd, std::uint8_t rs, std::uint8_t rt, std::size_t line = 1, std::size_t column = 1) {
    Instruction inst;
    inst.operation = common::Operation::SRA;
    inst.location = SourceLocation{line, column};
    inst.operands.push_back(RegisterOperand{rd});
    inst.operands.push_back(RegisterOperand{rs});
    inst.operands.push_back(RegisterOperand{rt});
    return inst;
}

Instruction makeMul(std::uint8_t rd, std::uint8_t rs, std::uint8_t rt, std::size_t line = 1, std::size_t column = 1) {
    Instruction inst;
    inst.operation = common::Operation::MUL;
    inst.location = SourceLocation{line, column};
    inst.operands.push_back(RegisterOperand{rd});
    inst.operands.push_back(RegisterOperand{rs});
    inst.operands.push_back(RegisterOperand{rt});
    return inst;
}

Instruction makeUdiv(std::uint8_t rd, std::uint8_t rs, std::uint8_t rt, std::size_t line = 1, std::size_t column = 1) {
    Instruction inst;
    inst.operation = common::Operation::UDIV;
    inst.location = SourceLocation{line, column};
    inst.operands.push_back(RegisterOperand{rd});
    inst.operands.push_back(RegisterOperand{rs});
    inst.operands.push_back(RegisterOperand{rt});
    return inst;
}

Instruction makeSdiv(std::uint8_t rd, std::uint8_t rs, std::uint8_t rt, std::size_t line = 1, std::size_t column = 1) {
    Instruction inst;
    inst.operation = common::Operation::SDIV;
    inst.location = SourceLocation{line, column};
    inst.operands.push_back(RegisterOperand{rd});
    inst.operands.push_back(RegisterOperand{rs});
    inst.operands.push_back(RegisterOperand{rt});
    return inst;
}

Instruction makeRegisterImmediate(
    common::Operation operation,
    std::uint8_t ra,
    std::uint8_t rb,
    std::uint32_t imm,
    std::size_t line = 1,
    std::size_t column = 1
) {
    Instruction inst;
    inst.operation = operation;
    inst.location = SourceLocation{line, column};
    inst.operands.push_back(RegisterOperand{ra});
    inst.operands.push_back(RegisterOperand{rb});
    inst.operands.push_back(ImmediateOperand{imm});
    return inst;
}

Instruction makeStackInstruction(
    common::Operation operation,
    Operand first,
    Operand second
) {
    Instruction inst;
    inst.operation = operation;
    inst.operands.push_back(std::move(first));
    inst.operands.push_back(std::move(second));
    return inst;
}

}

TEST_CASE("encoder encodes single LI instruction") {
    Program program;
    program.instructions.push_back(makeLi(0, 0x0001));

    Encoder encoder;
    const auto bytes = encoder.encode(program);

    REQUIRE(bytes.size() == 4);
    REQUIRE(bytes[0] == 0x00);
    REQUIRE(bytes[1] == 0x00);
    REQUIRE(bytes[2] == 0x01);
    REQUIRE(bytes[3] == 0x00);
}

TEST_CASE("encoder encodes LI with non-trivial immediate") {
    Program program;
    program.instructions.push_back(makeLi(1, 0x8000));

    Encoder encoder;
    const auto bytes = encoder.encode(program);

    REQUIRE(bytes.size() == 4);
    REQUIRE(bytes[0] == 0x00);
    REQUIRE(bytes[1] == 0x01);
    REQUIRE(bytes[2] == 0x00);
    REQUIRE(bytes[3] == 0x80);
}

TEST_CASE("encoder encodes single LUI instruction") {
    Program program;
    program.instructions.push_back(makeLui(2, 0xABCD));

    Encoder encoder;
    const auto bytes = encoder.encode(program);

    REQUIRE(bytes.size() == 4);
    REQUIRE(bytes[0] == 0x01);
    REQUIRE(bytes[1] == 0x02);
    REQUIRE(bytes[2] == 0xCD);
    REQUIRE(bytes[3] == 0xAB);
}

TEST_CASE("encoder encodes single ADD instruction") {
    Program program;
    program.instructions.push_back(makeAdd(1, 1, 0));

    Encoder encoder;
    const auto bytes = encoder.encode(program);

    REQUIRE(bytes.size() == 4);
    REQUIRE(bytes[0] == 0x02);
    REQUIRE(bytes[1] == 0x01);
    REQUIRE(bytes[2] == 0x01);
    REQUIRE(bytes[3] == 0x00);
}

TEST_CASE("encoder encodes single SUB instruction") {
    Program program;
    program.instructions.push_back(makeSub(1, 2, 3));

    Encoder encoder;
    const auto bytes = encoder.encode(program);

    REQUIRE(bytes.size() == 4);
    REQUIRE(bytes[0] == 0x03);
    REQUIRE(bytes[1] == 0x01);
    REQUIRE(bytes[2] == 0x02);
    REQUIRE(bytes[3] == 0x03);
}

TEST_CASE("encoder encodes single AND instruction") {
    Program program;
    program.instructions.push_back(makeAnd(1, 2, 3));

    Encoder encoder;
    const auto bytes = encoder.encode(program);

    REQUIRE(bytes.size() == 4);
    REQUIRE(bytes[0] == 0x04);
    REQUIRE(bytes[1] == 0x01);
    REQUIRE(bytes[2] == 0x02);
    REQUIRE(bytes[3] == 0x03);
}

TEST_CASE("encoder encodes single OR instruction") {
    Program program;
    program.instructions.push_back(makeOr(1, 2, 3));

    Encoder encoder;
    const auto bytes = encoder.encode(program);

    REQUIRE(bytes.size() == 4);
    REQUIRE(bytes[0] == 0x05);
    REQUIRE(bytes[1] == 0x01);
    REQUIRE(bytes[2] == 0x02);
    REQUIRE(bytes[3] == 0x03);
}

TEST_CASE("encoder encodes single XOR instruction") {
    Program program;
    program.instructions.push_back(makeXor(1, 2, 3));

    Encoder encoder;
    const auto bytes = encoder.encode(program);

    REQUIRE(bytes.size() == 4);
    REQUIRE(bytes[0] == 0x06);
    REQUIRE(bytes[1] == 0x01);
    REQUIRE(bytes[2] == 0x02);
    REQUIRE(bytes[3] == 0x03);
}

TEST_CASE("encoder encodes single SLL instruction") {
    Program program;
    program.instructions.push_back(makeSll(1, 2, 3));

    Encoder encoder;
    const auto bytes = encoder.encode(program);

    REQUIRE(bytes.size() == 4);
    REQUIRE(bytes[0] == 0x07);
    REQUIRE(bytes[1] == 0x01);
    REQUIRE(bytes[2] == 0x02);
    REQUIRE(bytes[3] == 0x03);
}

TEST_CASE("encoder encodes single SRL instruction") {
    Program program;
    program.instructions.push_back(makeSrl(1, 2, 3));

    Encoder encoder;
    const auto bytes = encoder.encode(program);

    REQUIRE(bytes.size() == 4);
    REQUIRE(bytes[0] == 0x08);
    REQUIRE(bytes[1] == 0x01);
    REQUIRE(bytes[2] == 0x02);
    REQUIRE(bytes[3] == 0x03);
}

TEST_CASE("encoder encodes single SRA instruction") {
    Program program;
    program.instructions.push_back(makeSra(1, 2, 3));

    Encoder encoder;
    const auto bytes = encoder.encode(program);

    REQUIRE(bytes.size() == 4);
    REQUIRE(bytes[0] == 0x09);
    REQUIRE(bytes[1] == 0x01);
    REQUIRE(bytes[2] == 0x02);
    REQUIRE(bytes[3] == 0x03);
}

TEST_CASE("encoder encodes single MUL instruction") {
    Program program;
    program.instructions.push_back(makeMul(1, 2, 3));

    Encoder encoder;
    const auto bytes = encoder.encode(program);

    REQUIRE(bytes.size() == 4);
    REQUIRE(bytes[0] == 0x0A);
    REQUIRE(bytes[1] == 0x01);
    REQUIRE(bytes[2] == 0x02);
    REQUIRE(bytes[3] == 0x03);
}

TEST_CASE("encoder encodes single UDIV instruction") {
    Program program;
    program.instructions.push_back(makeUdiv(1, 2, 3));

    Encoder encoder;
    const auto bytes = encoder.encode(program);

    REQUIRE(bytes.size() == 4);
    REQUIRE(bytes[0] == 0x0B);
    REQUIRE(bytes[1] == 0x01);
    REQUIRE(bytes[2] == 0x02);
    REQUIRE(bytes[3] == 0x03);
}

TEST_CASE("encoder encodes single SDIV instruction") {
    Program program;
    program.instructions.push_back(makeSdiv(1, 2, 3));

    Encoder encoder;
    const auto bytes = encoder.encode(program);

    REQUIRE(bytes.size() == 4);
    REQUIRE(bytes[0] == 0x0C);
    REQUIRE(bytes[1] == 0x01);
    REQUIRE(bytes[2] == 0x02);
    REQUIRE(bytes[3] == 0x03);
}

TEST_CASE("encoder encodes memory-format instructions") {
    Program program;
    program.instructions.push_back(makeRegisterImmediate(common::Operation::LDB, 1, 2, 0x80));
    program.instructions.push_back(makeRegisterImmediate(common::Operation::LDH, 2, 3, 0x81));
    program.instructions.push_back(makeRegisterImmediate(common::Operation::LDW, 3, 4, 0x82));
    program.instructions.push_back(makeRegisterImmediate(common::Operation::STB, 4, 5, 0x83));
    program.instructions.push_back(makeRegisterImmediate(common::Operation::STH, 5, 6, 0x84));
    program.instructions.push_back(makeRegisterImmediate(common::Operation::STW, 6, 7, 0x85));
    program.instructions.push_back(makeRegisterImmediate(common::Operation::SXT, 7, 8, 0x86));
    program.instructions.push_back(makeRegisterImmediate(common::Operation::BSWAP, 8, 9, 0x87));

    Encoder encoder;
    const auto bytes = encoder.encode(program);

    const std::vector<std::uint8_t> expected = {
        0x10, 0x01, 0x02, 0x80,
        0x11, 0x02, 0x03, 0x81,
        0x12, 0x03, 0x04, 0x82,
        0x13, 0x04, 0x05, 0x83,
        0x14, 0x05, 0x06, 0x84,
        0x15, 0x06, 0x07, 0x85,
        0x16, 0x07, 0x08, 0x86,
        0x17, 0x08, 0x09, 0x87
    };

    REQUIRE(bytes == expected);
}

TEST_CASE("encoder encodes stack instructions") {
    Program program;
    program.instructions.push_back(makeStackInstruction(
        common::Operation::SCRW,
        StatusRegisterOperand{common::StatusRegister::SpTop},
        RegisterOperand{5}
    ));
    program.instructions.push_back(makeStackInstruction(
        common::Operation::SCRR,
        RegisterOperand{5},
        StatusRegisterOperand{common::StatusRegister::SpTop}
    ));
    program.instructions.push_back(makeStackInstruction(
        common::Operation::ASPI,
        RegisterOperand{5},
        ImmediateOperand{0xfff0}
    ));
    program.instructions.push_back(makeStackInstruction(
        common::Operation::ASPR,
        RegisterOperand{5},
        RegisterOperand{6}
    ));

    const std::vector<std::uint8_t> bytes = Encoder{}.encode(program);
    const std::vector<std::uint8_t> expected = {
        0x20, 0x01, 0x05, 0x00,
        0x21, 0x05, 0x01, 0x00,
        0x22, 0x05, 0xf0, 0xff,
        0x23, 0x05, 0x06, 0x00
    };

    REQUIRE(bytes == expected);
}

TEST_CASE("encoder encodes full sample program") {
    Program program;
    program.instructions.push_back(makeLi(0, 0x0001));
    program.instructions.push_back(makeLi(1, 0x8000));
    program.instructions.push_back(makeAdd(1, 1, 0));

    Encoder encoder;
    const auto bytes = encoder.encode(program);

    const std::vector<std::uint8_t> expected = {
        0x00, 0x00, 0x01, 0x00,
        0x00, 0x01, 0x00, 0x80,
        0x02, 0x01, 0x01, 0x00
    };

    REQUIRE(bytes == expected);
}

TEST_CASE("encoder encodes arithmetic program") {
    Program program;
    program.instructions.push_back(makeLi(0, 0x0010));
    program.instructions.push_back(makeLi(1, 0x0004));
    program.instructions.push_back(makeSub(2, 0, 1));
    program.instructions.push_back(makeMul(3, 2, 1));
    program.instructions.push_back(makeUdiv(4, 3, 1));
    program.instructions.push_back(makeSdiv(5, 2, 1));

    Encoder encoder;
    const auto bytes = encoder.encode(program);

    const std::vector<std::uint8_t> expected = {
        0x00, 0x00, 0x10, 0x00,
        0x00, 0x01, 0x04, 0x00,
        0x03, 0x02, 0x00, 0x01,
        0x0A, 0x03, 0x02, 0x01,
        0x0B, 0x04, 0x03, 0x01,
        0x0C, 0x05, 0x02, 0x01
    };

    REQUIRE(bytes == expected);
}

TEST_CASE("encoder encodes bitwise and shift program") {
    Program program;
    program.instructions.push_back(makeLui(0, 0x8000));
    program.instructions.push_back(makeLi(1, 0x0001));
    program.instructions.push_back(makeAnd(2, 0, 1));
    program.instructions.push_back(makeOr(3, 0, 1));
    program.instructions.push_back(makeXor(4, 0, 1));
    program.instructions.push_back(makeSll(5, 1, 1));
    program.instructions.push_back(makeSrl(6, 0, 1));
    program.instructions.push_back(makeSra(7, 0, 1));

    Encoder encoder;
    const auto bytes = encoder.encode(program);

    const std::vector<std::uint8_t> expected = {
        0x01, 0x00, 0x00, 0x80,
        0x00, 0x01, 0x01, 0x00,
        0x04, 0x02, 0x00, 0x01,
        0x05, 0x03, 0x00, 0x01,
        0x06, 0x04, 0x00, 0x01,
        0x07, 0x05, 0x01, 0x01,
        0x08, 0x06, 0x00, 0x01,
        0x09, 0x07, 0x00, 0x01
    };

    REQUIRE(bytes == expected);
}

TEST_CASE("encoder rejects LI with wrong operand count") {
    Instruction inst;
    inst.operation = common::Operation::LI;
    inst.location = SourceLocation{3, 5};
    inst.operands.push_back(RegisterOperand{0});

    Program program;
    program.instructions.push_back(inst);

    Encoder encoder;

    REQUIRE_THROWS_WITH(
        encoder.encode(program),
        ContainsSubstring("LI expects 2 operands")
    );
}

TEST_CASE("encoder rejects ADD with wrong operand count") {
    Instruction inst;
    inst.operation = common::Operation::ADD;
    inst.location = SourceLocation{4, 2};
    inst.operands.push_back(RegisterOperand{1});
    inst.operands.push_back(RegisterOperand{2});

    Program program;
    program.instructions.push_back(inst);

    Encoder encoder;

    REQUIRE_THROWS_WITH(
        encoder.encode(program),
        ContainsSubstring("ADD expects 3 operands")
    );
}

TEST_CASE("encoder rejects LI when destination is not register") {
    Instruction inst;
    inst.operation = common::Operation::LI;
    inst.location = SourceLocation{5, 1};
    inst.operands.push_back(ImmediateOperand{7});
    inst.operands.push_back(ImmediateOperand{9});

    Program program;
    program.instructions.push_back(inst);

    Encoder encoder;

    REQUIRE_THROWS_WITH(
        encoder.encode(program),
        ContainsSubstring("destination register must be a register")
    );
}

TEST_CASE("encoder rejects LI when immediate is not immediate") {
    Instruction inst;
    inst.operation = common::Operation::LI;
    inst.location = SourceLocation{6, 1};
    inst.operands.push_back(RegisterOperand{2});
    inst.operands.push_back(RegisterOperand{3});

    Program program;
    program.instructions.push_back(inst);

    Encoder encoder;

    REQUIRE_THROWS_WITH(
        encoder.encode(program),
        ContainsSubstring("immediate must be an immediate")
    );
}

TEST_CASE("encoder rejects ADD when one operand is not register") {
    Instruction inst;
    inst.operation = common::Operation::ADD;
    inst.location = SourceLocation{7, 1};
    inst.operands.push_back(RegisterOperand{1});
    inst.operands.push_back(ImmediateOperand{5});
    inst.operands.push_back(RegisterOperand{0});

    Program program;
    program.instructions.push_back(inst);

    Encoder encoder;

    REQUIRE_THROWS_WITH(
        encoder.encode(program),
        ContainsSubstring("left source register must be a register")
    );
}

TEST_CASE("encoder rejects RRR instruction when right source is not register") {
    Instruction inst;
    inst.operation = common::Operation::XOR;
    inst.location = SourceLocation{8, 1};
    inst.operands.push_back(RegisterOperand{1});
    inst.operands.push_back(RegisterOperand{2});
    inst.operands.push_back(ImmediateOperand{3});

    Program program;
    program.instructions.push_back(inst);

    Encoder encoder;

    REQUIRE_THROWS_WITH(
        encoder.encode(program),
        ContainsSubstring("right source register must be a register")
    );
}

TEST_CASE("encoder rejects immediate larger than 16 bits") {
    Instruction inst;
    inst.operation = common::Operation::LUI;
    inst.location = SourceLocation{9, 1};
    inst.operands.push_back(RegisterOperand{1});
    inst.operands.push_back(ImmediateOperand{0x10000});

    Program program;
    program.instructions.push_back(inst);

    Encoder encoder;

    REQUIRE_THROWS_WITH(
        encoder.encode(program),
        ContainsSubstring("immediate must fit into 16 bits")
    );
}

TEST_CASE("encoder rejects memory-format instruction with immediate larger than 8 bits") {
    Program program;
    program.instructions.push_back(makeRegisterImmediate(common::Operation::LDW, 1, 2, 0x100));

    Encoder encoder;

    REQUIRE_THROWS_WITH(
        encoder.encode(program),
        ContainsSubstring("immediate must fit into 8 bits")
    );
}

TEST_CASE("encoder rejects invalid status register index") {
    Program program;
    program.instructions.push_back(makeStackInstruction(
        common::Operation::SCRW,
        StatusRegisterOperand{static_cast<common::StatusRegister>(6)},
        RegisterOperand{5}
    ));

    REQUIRE_THROWS_WITH(
        Encoder{}.encode(program),
        ContainsSubstring("status register has invalid index")
    );
}

TEST_CASE("encoder rejects ASPI immediate larger than 16 bits") {
    Program program;
    program.instructions.push_back(makeStackInstruction(
        common::Operation::ASPI,
        RegisterOperand{5},
        ImmediateOperand{0x10000}
    ));

    REQUIRE_THROWS_WITH(
        Encoder{}.encode(program),
        ContainsSubstring("immediate must fit into 16 bits")
    );
}

TEST_CASE("encoder rejects stack instruction with wrong operand count") {
    Instruction instruction;
    instruction.operation = common::Operation::ASPR;
    instruction.operands.push_back(RegisterOperand{5});

    Program program;
    program.instructions.push_back(std::move(instruction));

    REQUIRE_THROWS_WITH(
        Encoder{}.encode(program),
        ContainsSubstring("ASPR expects 2 operands")
    );
}

TEST_CASE("encoder rejects pseudo instruction that was not lowered") {
    Instruction inst;
    inst.operation = common::Operation::MOV;
    inst.location = SourceLocation{10, 1};
    inst.operands.push_back(RegisterOperand{1});
    inst.operands.push_back(RegisterOperand{2});

    Program program;
    program.instructions.push_back(inst);

    Encoder encoder;

    REQUIRE_THROWS_WITH(
        encoder.encode(program),
        ContainsSubstring("unknown operation during encoding")
    );
}

TEST_CASE("encoder encodes empty program into empty byte vector") {
    Program program;

    Encoder encoder;
    const auto bytes = encoder.encode(program);

    REQUIRE(bytes.empty());
}
