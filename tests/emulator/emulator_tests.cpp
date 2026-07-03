#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "emulator.h"

#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
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

TEST_CASE("emulator step after program completion is a no-op") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x2A, 0x00 // LI R0 42
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.run();

    const auto finishedPc = emulator.pc();
    const auto result = emulator.registers()[0];

    emulator.step();

    REQUIRE(emulator.pc() == finishedPc);
    REQUIRE(emulator.registers()[0] == result);
    REQUIRE(emulator.isFinished());
}

TEST_CASE("emulator executes program loaded at non-zero base address") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x01, 0x00 // LI R0 1
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program, 16);

    REQUIRE(emulator.pc() == 16u);
    REQUIRE_FALSE(emulator.isFinished());

    emulator.step();

    REQUIRE(emulator.registers()[0] == 1u);
    REQUIRE(emulator.pc() == 20u);
    REQUIRE(emulator.isFinished());
}

TEST_CASE("emulator loadProgram resets registers PC and temporary register") {
    const std::uint8_t temp = common::assemblerTempRegister;
    const std::vector<std::uint8_t> firstProgram = {
        0x00, 0x00, 0x07, 0x00, // LI R0 7
        0x00, temp, 0xFF, 0xFF  // LI temp 0xFFFF
    };
    const std::vector<std::uint8_t> secondProgram = {
        0x00, 0x02, 0x02, 0x00, // LI R2 2
        0x06, 0x01, 0x02, temp  // XOR R1 R2 temp
    };

    emulator::Emulator emulator;
    emulator.loadProgram(firstProgram);
    emulator.run();

    REQUIRE(emulator.registers()[0] == 7u);

    emulator.loadProgram(secondProgram);

    REQUIRE(emulator.pc() == common::resetAddress);
    for (const auto value : emulator.registers()) {
        REQUIRE(value == 0u);
    }

    emulator.run();

    REQUIRE(emulator.registers()[0] == 0u);
    REQUIRE(emulator.registers()[1] == 2u);
    REQUIRE(emulator.registers()[2] == 2u);
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

TEST_CASE("emulator arithmetic wraps modulo 32 bits") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0xFF, 0xFF, // LI R0 0xFFFF
        0x01, 0x00, 0xFF, 0xFF, // LUI R0 0xFFFF
        0x00, 0x01, 0x01, 0x00, // LI R1 1
        0x02, 0x02, 0x00, 0x01, // ADD R2 R0 R1
        0x00, 0x03, 0x00, 0x00, // LI R3 0
        0x03, 0x04, 0x03, 0x01, // SUB R4 R3 R1
        0x00, 0x05, 0x00, 0x00, // LI R5 0
        0x01, 0x05, 0x01, 0x00, // LUI R5 1
        0x0A, 0x06, 0x05, 0x05  // MUL R6 R5 R5
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.run();

    const auto& regs = emulator.registers();
    REQUIRE(regs[2] == 0u);
    REQUIRE(regs[4] == 0xFFFFFFFFu);
    REQUIRE(regs[6] == 0u);
}

TEST_CASE("emulator reads aliased source before writing destination") {
    const std::uint8_t temp = common::assemblerTempRegister;
    const std::vector<std::uint8_t> program = {
        0x00, 0x01, 0x05, 0x00, // LI R1 5
        0x00, temp, 0x00, 0x00, // LI temp 0
        0x03, 0x01, temp, 0x01  // SUB R1 temp R1
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.run();

    REQUIRE(emulator.registers()[1] == 0xFFFFFFFBu);
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

TEST_CASE("emulator handles shift boundary values") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x01, 0x00, // LI R0 1
        0x01, 0x00, 0x00, 0x80, // LUI R0 0x8000
        0x00, 0x01, 0x00, 0x00, // LI R1 0
        0x00, 0x02, 0x1F, 0x00, // LI R2 31
        0x09, 0x03, 0x00, 0x01, // SRA R3 R0 R1
        0x08, 0x04, 0x00, 0x02, // SRL R4 R0 R2
        0x09, 0x05, 0x00, 0x02, // SRA R5 R0 R2
        0x00, 0x06, 0x01, 0x00, // LI R6 1
        0x07, 0x07, 0x06, 0x02  // SLL R7 R6 R2
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.run();

    const auto& regs = emulator.registers();
    REQUIRE(regs[3] == 0x80000001u);
    REQUIRE(regs[4] == 0x00000001u);
    REQUIRE(regs[5] == 0xFFFFFFFFu);
    REQUIRE(regs[7] == 0x80000000u);
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

TEST_CASE("emulator validates every register field in RRR instruction") {
    SECTION("destination register") {
        const std::vector<std::uint8_t> program = {
            0x02, 0x10, 0x00, 0x01 // ADD R16 R0 R1
        };

        emulator::Emulator emulator;
        emulator.loadProgram(program);

        REQUIRE_THROWS_WITH(
            emulator.run(),
            ContainsSubstring("invalid register index in destination")
        );
    }

    SECTION("left source register") {
        const std::vector<std::uint8_t> program = {
            0x02, 0x00, 0x10, 0x01 // ADD R0 R16 R1
        };

        emulator::Emulator emulator;
        emulator.loadProgram(program);

        REQUIRE_THROWS_WITH(
            emulator.run(),
            ContainsSubstring("invalid register index in left source")
        );
    }

    SECTION("right source register") {
        const std::vector<std::uint8_t> program = {
            0x02, 0x00, 0x01, 0x10 // ADD R0 R1 R16
        };

        emulator::Emulator emulator;
        emulator.loadProgram(program);

        REQUIRE_THROWS_WITH(
            emulator.run(),
            ContainsSubstring("invalid register index in right source")
        );
    }
}

#if MCST_TRACING

TEST_CASE("emulator writes disassembly trace with post-state destination values") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x03, 0xEF, 0xBE, // LI R3 0xBEEF
        0x02, 0x01, 0x02, 0x03  // ADD R1 R2 R3
    };

    std::ostringstream trace;
    emulator::Emulator emulator;
    emulator.loadProgram(program, 16);
    emulator.enableDisasmTrace(trace);
    emulator.run();

    REQUIRE(
        trace.str() ==
        "---- 0 0x0 ----\n"
        "0xbeef0300 LI R3 (0xbeef) imm (0xbeef)\n"
        "---- 1 0x4 ----\n"
        "0x03020102 ADD R1 (0xbeef) R2 (0x0) R3 (0xbeef)\n"
    );
}

TEST_CASE("trace reads aliased source operands from pre-instruction state") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x01, 0x05, 0x00, // LI R1 5
        0x02, 0x01, 0x01, 0x01  // ADD R1 R1 R1
    };

    std::ostringstream trace;
    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.enableDisasmTrace(trace, {{1, 1}});
    emulator.run();

    REQUIRE(
        trace.str() ==
        "---- 1 0x4 ----\n"
        "0x01010102 ADD R1 (0xa) R1 (0x5) R1 (0x5)\n"
    );
}

TEST_CASE("trace tick ranges are inclusive and overlapping ranges are merged") {
    const auto ranges = emulator::parseTickRanges("7-10,0,3-5,4-8");

    REQUIRE(ranges.size() == 2);
    REQUIRE(ranges[0].begin == 0);
    REQUIRE(ranges[0].end == 0);
    REQUIRE(ranges[1].begin == 3);
    REQUIRE(ranges[1].end == 10);

    REQUIRE(emulator::containsTick(ranges, 0));
    REQUIRE_FALSE(emulator::containsTick(ranges, 1));
    REQUIRE_FALSE(emulator::containsTick(ranges, 2));
    REQUIRE(emulator::containsTick(ranges, 3));
    REQUIRE(emulator::containsTick(ranges, 10));
    REQUIRE_FALSE(emulator::containsTick(ranges, 11));
}

TEST_CASE("trace tick range parser rejects malformed ranges") {
    REQUIRE_THROWS_AS(emulator::parseTickRanges(""), std::invalid_argument);
    REQUIRE_THROWS_AS(emulator::parseTickRanges("1,,2"), std::invalid_argument);
    REQUIRE_THROWS_AS(emulator::parseTickRanges("5-2"), std::invalid_argument);
    REQUIRE_THROWS_AS(emulator::parseTickRanges("1-x"), std::invalid_argument);
}

#endif
