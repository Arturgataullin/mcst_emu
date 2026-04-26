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