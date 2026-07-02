#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "emulator.h"

#include <cstdint>
#include <vector>

using Catch::Matchers::ContainsSubstring;

TEST_CASE("emulator executes sample program") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x01, 0x00, // LI R0 0x0001
        0x00, 0x01, 0x00, 0x80, // LI R1 0x8000
        0x02, 0x01, 0x01, 0x00  // ADD R1 R1 R0
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.run();

    const auto& regs = emulator.registers();

    REQUIRE(regs[0] == 0x00000001u);
    REQUIRE(regs[1] == 0x00008001u);

    for (std::size_t i = 2; i < regs.size(); ++i) {
        REQUIRE(regs[i] == 0u);
    }
}

TEST_CASE("emulator step executes one instruction") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x01, 0x00, // LI R0 0x0001
        0x00, 0x01, 0x00, 0x80  // LI R1 0x8000
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);

    emulator.step();
    REQUIRE(emulator.registers()[0] == 1u);
    REQUIRE(emulator.registers()[1] == 0u);
    REQUIRE(emulator.pc() == 4u);

    emulator.step();
    REQUIRE(emulator.registers()[1] == 0x8000u);
    REQUIRE(emulator.pc() == 8u);
    REQUIRE(emulator.isFinished());
}

TEST_CASE("emulator executes instructions using assembler temporary register") {
    const std::uint8_t temp = common::assemblerTempRegister;
    const std::vector<std::uint8_t> program = {
        0x00, temp, 0xFF, 0xFF, // LI temp 0xFFFF
        0x01, temp, 0xFF, 0xFF, // LUI temp 0xFFFF
        0x06, 0x00, 0x00, temp  // XOR R0 R0 temp
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.run();

    REQUIRE(emulator.registers()[0] == 0xFFFFFFFFu);
}

TEST_CASE("assembler temporary register does not overlap user register R15") {
    const std::uint8_t temp = common::assemblerTempRegister;
    const std::vector<std::uint8_t> program = {
        0x00, 0x0F, 0xFF, 0x00, // LI R15 0x00FF
        0x00, temp, 0xFF, 0xFF, // LI temp 0xFFFF
        0x01, temp, 0xFF, 0xFF, // LUI temp 0xFFFF
        0x06, 0x0F, 0x0F, temp  // XOR R15 R15 temp
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.run();

    REQUIRE(emulator.registers()[15] == 0xFFFFFF00u);
}

TEST_CASE("emulator LUI preserves lower half of register") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x78, 0x56, // LI R0 0x5678
        0x01, 0x00, 0x34, 0x12  // LUI R0 0x1234
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.run();

    REQUIRE(emulator.registers()[0] == 0x12345678u);
}

TEST_CASE("emulator executes arithmetic and bitwise instructions") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x06, 0x00, // LI R0 6
        0x00, 0x01, 0x03, 0x00, // LI R1 3
        0x03, 0x02, 0x00, 0x01, // SUB R2 R0 R1
        0x0A, 0x03, 0x00, 0x01, // MUL R3 R0 R1
        0x04, 0x04, 0x00, 0x01, // AND R4 R0 R1
        0x05, 0x05, 0x00, 0x01, // OR R5 R0 R1
        0x06, 0x06, 0x00, 0x01  // XOR R6 R0 R1
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.run();

    const auto& regs = emulator.registers();
    REQUIRE(regs[2] == 3u);
    REQUIRE(regs[3] == 18u);
    REQUIRE(regs[4] == 2u);
    REQUIRE(regs[5] == 7u);
    REQUIRE(regs[6] == 5u);
}

TEST_CASE("emulator executes logical and arithmetic shifts") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x01, 0x00, // LI R0 1
        0x00, 0x01, 0x01, 0x00, // LI R1 1
        0x07, 0x02, 0x00, 0x01, // SLL R2 R0 R1
        0x00, 0x03, 0x00, 0x00, // LI R3 0
        0x01, 0x03, 0x00, 0x80, // LUI R3 0x8000
        0x08, 0x04, 0x03, 0x01, // SRL R4 R3 R1
        0x09, 0x05, 0x03, 0x01, // SRA R5 R3 R1
        0x00, 0x06, 0x20, 0x00, // LI R6 32
        0x07, 0x07, 0x00, 0x06  // SLL R7 R0 R6
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.run();

    const auto& regs = emulator.registers();
    REQUIRE(regs[2] == 0x00000002u);
    REQUIRE(regs[4] == 0x40000000u);
    REQUIRE(regs[5] == 0xC0000000u);
    REQUIRE(regs[7] == 0x00000001u);
}

TEST_CASE("emulator executes unsigned and signed division") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x07, 0x00, // LI R0 7
        0x00, 0x01, 0x02, 0x00, // LI R1 2
        0x0B, 0x02, 0x00, 0x01, // UDIV R2 R0 R1
        0x00, 0x03, 0xF8, 0xFF, // LI R3 0xFFF8
        0x01, 0x03, 0xFF, 0xFF, // LUI R3 0xFFFF
        0x0C, 0x04, 0x03, 0x01, // SDIV R4 R3 R1
        0x00, 0x05, 0x00, 0x00, // LI R5 0
        0x01, 0x05, 0x00, 0x80, // LUI R5 0x8000
        0x00, 0x06, 0xFF, 0xFF, // LI R6 0xFFFF
        0x01, 0x06, 0xFF, 0xFF, // LUI R6 0xFFFF
        0x0C, 0x07, 0x05, 0x06  // SDIV R7 R5 R6
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.run();

    const auto& regs = emulator.registers();
    REQUIRE(regs[2] == 3u);
    REQUIRE(regs[4] == 0xFFFFFFFCu);
    REQUIRE(regs[7] == 0x80000000u);
}

TEST_CASE("emulator rejects division by zero") {
    SECTION("unsigned division") {
        const std::vector<std::uint8_t> program = {
            0x00, 0x00, 0x01, 0x00, // LI R0 1
            0x00, 0x01, 0x00, 0x00, // LI R1 0
            0x0B, 0x02, 0x00, 0x01  // UDIV R2 R0 R1
        };

        emulator::Emulator emulator;
        emulator.loadProgram(program);

        REQUIRE_THROWS_WITH(
            emulator.run(),
            ContainsSubstring("UDIV: division by zero")
        );
    }

    SECTION("signed division") {
        const std::vector<std::uint8_t> program = {
            0x00, 0x00, 0x01, 0x00, // LI R0 1
            0x00, 0x01, 0x00, 0x00, // LI R1 0
            0x0C, 0x02, 0x00, 0x01  // SDIV R2 R0 R1
        };

        emulator::Emulator emulator;
        emulator.loadProgram(program);

        REQUIRE_THROWS_WITH(
            emulator.run(),
            ContainsSubstring("SDIV: division by zero")
        );
    }
}

TEST_CASE("emulator rejects invalid opcode") {
    const std::vector<std::uint8_t> program = {
        0xFF, 0x00, 0x00, 0x00
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);

    REQUIRE_THROWS_WITH(
        emulator.run(),
        ContainsSubstring("invalid opcode")
    );
}

TEST_CASE("emulator rejects program with invalid size") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x01 // LI R0 I0
    };

    emulator::Emulator emulator;

    REQUIRE_THROWS_WITH(
        emulator.loadProgram(program),
        ContainsSubstring("multiple of 4 bytes")
    );
}

TEST_CASE("emulator rejects invalid register index in binary") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x10, 0x01, 0x00 // LI R16 0x0001
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);

    REQUIRE_THROWS_WITH(
        emulator.run(),
        ContainsSubstring("invalid register index")
    );
}
