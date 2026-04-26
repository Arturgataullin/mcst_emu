#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "encoder.h"

#include <cstdint>
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

Instruction makeLi(std::uint8_t rd, std::uint16_t imm, std::size_t line = 1, std::size_t column = 1) {
    Instruction inst;
    inst.operation = common::Operation::LI;
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

TEST_CASE("encoder encodes empty program into empty byte vector") {
    Program program;

    Encoder encoder;
    const auto bytes = encoder.encode(program);

    REQUIRE(bytes.empty());
}