#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "emulator.h"

#include <cstdint>
#include <limits>
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

TEST_CASE("emulator supports program ending at top of 32-bit address space") {
    const std::size_t fullAddressSpace =
        static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) + 1u;
    const std::uint32_t lastInstructionAddress =
        std::numeric_limits<std::uint32_t>::max() - 3u;
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x01, 0x00 // LI R0 1
    };

    emulator::Emulator emulator(fullAddressSpace);
    emulator.loadProgram(program, lastInstructionAddress);

    REQUIRE_FALSE(emulator.isFinished());

    emulator.step();

    REQUIRE(emulator.registers()[0] == 1u);
    REQUIRE(emulator.pc() == (std::uint64_t{1} << 32));
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

TEST_CASE("emulator executes memory load and store instructions") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x40, 0x00, // LI R0 64
        0x00, 0x01, 0x44, 0x33, // LI R1 0x3344
        0x01, 0x01, 0x22, 0x11, // LUI R1 0x1122
        0x15, 0x01, 0x00, 0x01, // STW R1 R0 1
        0x10, 0x02, 0x00, 0x01, // LDB R2 R0 1
        0x11, 0x03, 0x00, 0x02, // LDH R3 R0 2
        0x12, 0x04, 0x00, 0x01, // LDW R4 R0 1
        0x00, 0x05, 0xCD, 0xAB, // LI R5 0xABCD
        0x13, 0x05, 0x00, 0x05, // STB R5 R0 5
        0x14, 0x05, 0x00, 0x06, // STH R5 R0 6
        0x12, 0x06, 0x00, 0x05  // LDW R6 R0 5
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.run();

    const auto& regs = emulator.registers();
    REQUIRE(regs[2] == 0x00000044u);
    REQUIRE(regs[3] == 0x00002233u);
    REQUIRE(regs[4] == 0x11223344u);
    REQUIRE(regs[6] == 0x00ABCDCDu);
}

TEST_CASE("emulator executes SXT and BSWAP instructions") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x01, 0xDD, 0xCC, // LI R1 0xCCDD
        0x01, 0x01, 0xBB, 0xAA, // LUI R1 0xAABB
        0x16, 0x02, 0x01, 0x00, // SXT R2 R1 0
        0x16, 0x03, 0x01, 0x01, // SXT R3 R1 1
        0x16, 0x04, 0x01, 0x02, // SXT R4 R1 2
        0x17, 0x05, 0x01, 0x01, // BSWAP R5 R1 1
        0x17, 0x06, 0x01, 0x02, // BSWAP R6 R1 2
        0x17, 0x07, 0x01, 0x00  // BSWAP R7 R1 0
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.run();

    const auto& regs = emulator.registers();
    REQUIRE(regs[1] == 0xAABBCCDDu);
    REQUIRE(regs[2] == 0xFFFFFFDDu);
    REQUIRE(regs[3] == 0xFFFFCCDDu);
    REQUIRE(regs[4] == 0xAABBCCDDu);
    REQUIRE(regs[5] == 0xAABBDDCCu);
    REQUIRE(regs[6] == 0xDDCCBBAAu);
    REQUIRE(regs[7] == 0xAABBCCDDu);
}

TEST_CASE("emulator warns on uninitialized RAM read") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x40, 0x00, // LI R0 64
        0x10, 0x01, 0x00, 0x02  // LDB R1 R0 2
    };

    std::ostringstream warnings;
    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.enableUninitializedRamWarnings(warnings);
    emulator.run();

    REQUIRE(
        warnings.str() ==
        "1 WARN uninit-ram pc=0x00000004 read 1 byte(s) at 0x00000042 uninit: 0x00000042\n"
    );
    REQUIRE(emulator.registers()[1] == 0u);
}

TEST_CASE("emulator warning lists only uninitialized bytes from a multi-byte read") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x40, 0x00, // LI R0 64
        0x00, 0x01, 0xAB, 0x00, // LI R1 0xAB
        0x13, 0x01, 0x00, 0x01, // STB R1 R0 1
        0x12, 0x02, 0x00, 0x00  // LDW R2 R0 0
    };

    std::ostringstream warnings;
    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.enableUninitializedRamWarnings(warnings);
    emulator.run();

    REQUIRE(
        warnings.str() ==
        "3 WARN uninit-ram pc=0x0000000c read 4 byte(s) at 0x00000040 uninit: "
        "0x00000040,0x00000042,0x00000043\n"
    );
    REQUIRE(emulator.registers()[2] == 0x0000AB00u);
}

TEST_CASE("emulator writes and reads stack status registers independently") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x34, 0x12, // LI R0 0x1234
        0x20, 0x01, 0x00, 0x00, // SCRW SP_TOP R0
        0x00, 0x00, 0xCD, 0xAB, // LI R0 0xABCD
        0x20, 0x02, 0x00, 0x00, // SCRW SP_SIZE R0
        0x21, 0x01, 0x01, 0x00, // SCRR R1 SP_TOP
        0x21, 0x02, 0x02, 0x00  // SCRR R2 SP_SIZE
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.run();

    REQUIRE(emulator.registers()[1] == 0x1234u);
    REQUIRE(emulator.registers()[2] == 0xABCDu);
    REQUIRE(emulator.pc() == program.size());
}

TEST_CASE("emulator executes conditional branches") {
    const std::vector<std::uint8_t> forwardBranch = {
        0x00, 0x00, 0x00, 0x00, // LI R0 0
        0x00, 0x01, 0x01, 0x00, // LI R1 1
        0x31, 0x00, 0x02, 0x00, // BRZ R0 2
        0x00, 0x01, 0x09, 0x00, // LI R1 9
        0x00, 0x02, 0x07, 0x00  // LI R2 7
    };

    emulator::Emulator forward;
    forward.loadProgram(forwardBranch);
    forward.run();

    REQUIRE(forward.registers()[1] == 1u);
    REQUIRE(forward.registers()[2] == 7u);

    const std::vector<std::uint8_t> backwardBranch = {
        0x00, 0x01, 0x03, 0x00, // LI R1 3
        0x00, 0x02, 0x01, 0x00, // LI R2 1
        0x03, 0x01, 0x01, 0x02, // SUB R1 R1 R2
        0x32, 0x01, 0xfe, 0xff, // BRNZ R1 -2
        0x00, 0x03, 0x09, 0x00  // LI R3 9
    };

    emulator::Emulator backward;
    backward.loadProgram(backwardBranch);
    backward.run();

    REQUIRE(backward.registers()[1] == 0u);
    REQUIRE(backward.registers()[3] == 9u);
}

TEST_CASE("emulator executes relative jump absolute jump and call") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x10, 0x00, // LI R0 16
        0x34, 0x00, 0x00, 0x00, // CALL R0
        0x00, 0x04, 0x05, 0x00, // LI R4 5
        0x30, 0x00, 0x03, 0x00, // RJMP 3
        0x00, 0x03, 0x07, 0x00, // LI R3 7
        0x33, 0x01, 0x00, 0x00  // AJMP R1
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.run();

    REQUIRE(emulator.registers()[common::returnAddressRegister] == 8u);
    REQUIRE(emulator.registers()[3] == 7u);
    REQUIRE(emulator.registers()[4] == 5u);
    REQUIRE(emulator.pc() == program.size());
}

TEST_CASE("emulator rejects jump before loaded program base") {
    const std::vector<std::uint8_t> program = {
        0x30, 0x00, 0xff, 0xff // RJMP -1
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program, 16);

    REQUIRE_THROWS_WITH(
        emulator.run(),
        ContainsSubstring("instruction fetch goes past loaded program")
    );
}

TEST_CASE("emulator executes comparison instructions") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x05, 0x00, // LI R0 5
        0x00, 0x01, 0x07, 0x00, // LI R1 7
        0x40, 0x02, 0x00, 0x01, // EQ R2 R0 R1
        0x41, 0x03, 0x00, 0x01, // NE R3 R0 R1
        0x42, 0x04, 0x00, 0x01, // LT R4 R0 R1
        0x43, 0x05, 0x00, 0x01, // GE R5 R0 R1
        0x00, 0x06, 0xff, 0xff, // LI R6 0xffff
        0x01, 0x06, 0xff, 0xff, // LUI R6 0xffff
        0x00, 0x07, 0x01, 0x00, // LI R7 1
        0x44, 0x08, 0x06, 0x07, // SLT R8 R6 R7
        0x45, 0x09, 0x06, 0x07  // SGE R9 R6 R7
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.run();

    REQUIRE(emulator.registers()[2] == 0u);
    REQUIRE(emulator.registers()[3] == 1u);
    REQUIRE(emulator.registers()[4] == 1u);
    REQUIRE(emulator.registers()[5] == 0u);
    REQUIRE(emulator.registers()[8] == 1u);
    REQUIRE(emulator.registers()[9] == 0u);
}

TEST_CASE("emulator resets stack status registers when loading another program") {
    const std::vector<std::uint8_t> firstProgram = {
        0x00, 0x00, 0x00, 0x01, // LI R0 0x0100
        0x20, 0x01, 0x00, 0x00, // SCRW SP_TOP R0
        0x00, 0x00, 0x20, 0x00, // LI R0 0x20
        0x20, 0x02, 0x00, 0x00  // SCRW SP_SIZE R0
    };
    const std::vector<std::uint8_t> secondProgram = {
        0x21, 0x01, 0x01, 0x00, // SCRR R1 SP_TOP
        0x21, 0x02, 0x02, 0x00  // SCRR R2 SP_SIZE
    };

    emulator::Emulator emulator;
    emulator.loadProgram(firstProgram);
    emulator.run();

    emulator.loadProgram(secondProgram);
    emulator.run();

    REQUIRE(emulator.registers()[1] == 0u);
    REQUIRE(emulator.registers()[2] == 0u);
}

TEST_CASE("emulator validates SCRW and SCRR operands") {
    SECTION("SCRW status register") {
        const std::vector<std::uint8_t> program = {
            0x20, 0x00, 0x00, 0x00 // SCRW SCR0 R0
        };

        emulator::Emulator emulator;
        emulator.loadProgram(program);
        REQUIRE_THROWS_WITH(emulator.run(), ContainsSubstring("invalid status register index"));
    }

    SECTION("SCRR status register") {
        const std::vector<std::uint8_t> program = {
            0x21, 0x00, 0x00, 0x00 // SCRR R0 SCR0
        };

        emulator::Emulator emulator;
        emulator.loadProgram(program);
        REQUIRE_THROWS_WITH(emulator.run(), ContainsSubstring("invalid status register index"));
    }

    SECTION("SCRW source register") {
        const std::vector<std::uint8_t> program = {
            0x20, 0x01, 0x10, 0x00 // SCRW SP_TOP R16
        };

        emulator::Emulator emulator;
        emulator.loadProgram(program);
        REQUIRE_THROWS_WITH(emulator.run(), ContainsSubstring("invalid register index in source"));
    }

    SECTION("SCRR destination register") {
        const std::vector<std::uint8_t> program = {
            0x21, 0x10, 0x01, 0x00 // SCRR R16 SP_TOP
        };

        emulator::Emulator emulator;
        emulator.loadProgram(program);
        REQUIRE_THROWS_WITH(emulator.run(), ContainsSubstring("invalid register index in destination"));
    }
}

TEST_CASE("ASPI allocates one byte and returns the new stack top") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x00, 0x01, // LI R0 0x0100
        0x20, 0x01, 0x00, 0x00, // SCRW SP_TOP R0
        0x00, 0x00, 0x01, 0x00, // LI R0 1
        0x20, 0x02, 0x00, 0x00, // SCRW SP_SIZE R0
        0x22, 0x03, 0xFF, 0xFF, // ASPI R3 0xFFFF (-1)
        0x21, 0x04, 0x01, 0x00, // SCRR R4 SP_TOP
        0x21, 0x05, 0x02, 0x00  // SCRR R5 SP_SIZE
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.run();

    REQUIRE(emulator.registers()[3] == 0xFFu);
    REQUIRE(emulator.registers()[4] == 0xFFu);
    REQUIRE(emulator.registers()[5] == 0u);
}

TEST_CASE("ASPI can allocate all available stack space") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x00, 0x01, // LI R0 0x0100
        0x20, 0x01, 0x00, 0x00, // SCRW SP_TOP R0
        0x00, 0x00, 0x10, 0x00, // LI R0 0x10
        0x20, 0x02, 0x00, 0x00, // SCRW SP_SIZE R0
        0x22, 0x03, 0xF0, 0xFF, // ASPI R3 0xFFF0 (-16)
        0x21, 0x04, 0x02, 0x00  // SCRR R4 SP_SIZE
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.run();

    REQUIRE(emulator.registers()[3] == 0xF0u);
    REQUIRE(emulator.registers()[4] == 0u);
}

TEST_CASE("ASPI rejects allocation larger than available stack space") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x00, 0x01, // LI R0 0x0100
        0x20, 0x01, 0x00, 0x00, // SCRW SP_TOP R0
        0x00, 0x00, 0x0F, 0x00, // LI R0 0x0F
        0x20, 0x02, 0x00, 0x00, // SCRW SP_SIZE R0
        0x22, 0x03, 0xF0, 0xFF  // ASPI R3 0xFFF0 (-16)
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);

    REQUIRE_THROWS_WITH(emulator.run(), ContainsSubstring("stack overflow"));
}

TEST_CASE("ASPI releases stack space") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x00, 0x01, // LI R0 0x0100
        0x20, 0x01, 0x00, 0x00, // SCRW SP_TOP R0
        0x00, 0x00, 0x10, 0x00, // LI R0 0x10
        0x20, 0x02, 0x00, 0x00, // SCRW SP_SIZE R0
        0x22, 0x02, 0xF0, 0xFF, // ASPI R2 -16
        0x22, 0x03, 0x10, 0x00, // ASPI R3 0x0010 (+16)
        0x21, 0x04, 0x02, 0x00  // SCRR R4 SP_SIZE
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.run();

    REQUIRE(emulator.registers()[3] == 0x100u);
    REQUIRE(emulator.registers()[4] == 0x10u);
}

TEST_CASE("ASPI rejects release past the beginning of the stack") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x00, 0x01, // LI R0 0x0100
        0x20, 0x01, 0x00, 0x00, // SCRW SP_TOP R0
        0x22, 0x03, 0x01, 0x00  // ASPI R3 1
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);

    REQUIRE_THROWS_WITH(emulator.run(), ContainsSubstring("stack underflow"));
}

TEST_CASE("released stack bytes are preserved when clear stack is disabled") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x40, 0x00, // LI R0 0x40
        0x20, 0x01, 0x00, 0x00, // SCRW SP_TOP R0
        0x00, 0x00, 0x04, 0x00, // LI R0 4
        0x20, 0x02, 0x00, 0x00, // SCRW SP_SIZE R0
        0x22, 0x01, 0xFC, 0xFF, // ASPI R1 -4
        0x00, 0x02, 0xCD, 0xAB, // LI R2 0xABCD
        0x15, 0x02, 0x01, 0x00, // STW R2 R1 0
        0x22, 0x03, 0x04, 0x00, // ASPI R3 4
        0x12, 0x04, 0x01, 0x00  // LDW R4 R1 0
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.run();

    REQUIRE(emulator.registers()[4] == 0x0000ABCDu);
}

TEST_CASE("released stack bytes become uninitialized without being cleared") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x40, 0x00, // LI R0 0x40
        0x20, 0x01, 0x00, 0x00, // SCRW SP_TOP R0
        0x00, 0x00, 0x04, 0x00, // LI R0 4
        0x20, 0x02, 0x00, 0x00, // SCRW SP_SIZE R0
        0x22, 0x01, 0xFC, 0xFF, // ASPI R1 -4
        0x00, 0x02, 0xCD, 0xAB, // LI R2 0xABCD
        0x15, 0x02, 0x01, 0x00, // STW R2 R1 0
        0x22, 0x03, 0x04, 0x00, // ASPI R3 4
        0x12, 0x04, 0x01, 0x00  // LDW R4 R1 0
    };
    std::ostringstream warnings;

    emulator::Emulator emulator;
    emulator.enableUninitializedRamWarnings(warnings);
    emulator.loadProgram(program);
    emulator.run();

    REQUIRE(emulator.registers()[4] == 0x0000ABCDu);
    REQUIRE(warnings.str().find("read 4 byte(s) at 0x0000003c") != std::string::npos);
}

TEST_CASE("clear stack zeroes released bytes and resets their initialization state") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x40, 0x00, // LI R0 0x40
        0x20, 0x01, 0x00, 0x00, // SCRW SP_TOP R0
        0x00, 0x00, 0x04, 0x00, // LI R0 4
        0x20, 0x02, 0x00, 0x00, // SCRW SP_SIZE R0
        0x22, 0x01, 0xFC, 0xFF, // ASPI R1 -4
        0x00, 0x02, 0xCD, 0xAB, // LI R2 0xABCD
        0x15, 0x02, 0x01, 0x00, // STW R2 R1 0
        0x22, 0x03, 0x04, 0x00, // ASPI R3 4
        0x12, 0x04, 0x01, 0x00  // LDW R4 R1 0
    };
    std::ostringstream warnings;

    emulator::Emulator emulator;
    emulator.enableClearStack();
    emulator.enableUninitializedRamWarnings(warnings);
    emulator.loadProgram(program);
    emulator.run();

    REQUIRE(emulator.registers()[4] == 0u);
    REQUIRE(warnings.str().find("read 4 byte(s) at 0x0000003c") != std::string::npos);
    REQUIRE(warnings.str().find("0x0000003c,0x0000003d,0x0000003e,0x0000003f") != std::string::npos);
}

TEST_CASE("ASPI supports multiple sequential allocations") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x00, 0x01, // LI R0 0x0100
        0x20, 0x01, 0x00, 0x00, // SCRW SP_TOP R0
        0x00, 0x00, 0x20, 0x00, // LI R0 0x20
        0x20, 0x02, 0x00, 0x00, // SCRW SP_SIZE R0
        0x22, 0x03, 0xFF, 0xFF, // ASPI R3 -1
        0x22, 0x04, 0xFE, 0xFF, // ASPI R4 -2
        0x21, 0x05, 0x02, 0x00  // SCRR R5 SP_SIZE
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.run();

    REQUIRE(emulator.registers()[3] == 0xFFu);
    REQUIRE(emulator.registers()[4] == 0xFDu);
    REQUIRE(emulator.registers()[5] == 0x1Du);
}

TEST_CASE("ASPR interprets 0xFFFFFFFF as minus one") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x00, 0x01, // LI R0 0x0100
        0x20, 0x01, 0x00, 0x00, // SCRW SP_TOP R0
        0x00, 0x00, 0x10, 0x00, // LI R0 0x10
        0x20, 0x02, 0x00, 0x00, // SCRW SP_SIZE R0
        0x00, 0x01, 0xFF, 0xFF, // LI R1 0xFFFF
        0x01, 0x01, 0xFF, 0xFF, // LUI R1 0xFFFF
        0x23, 0x03, 0x01, 0x00  // ASPR R3 R1
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.run();

    REQUIRE(emulator.registers()[3] == 0xFFu);
}

TEST_CASE("ASPR handles INT32_MIN without host signed overflow") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x00, 0x00, // LI R0 0
        0x01, 0x00, 0x00, 0x80, // LUI R0 0x8000
        0x20, 0x01, 0x00, 0x00, // SCRW SP_TOP R0
        0x20, 0x02, 0x00, 0x00, // SCRW SP_SIZE R0
        0x00, 0x01, 0x00, 0x00, // LI R1 0
        0x01, 0x01, 0x00, 0x80, // LUI R1 0x8000
        0x23, 0x03, 0x01, 0x00, // ASPR R3 R1
        0x21, 0x04, 0x02, 0x00  // SCRR R4 SP_SIZE
    };

    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.run();

    REQUIRE(emulator.registers()[3] == 0u);
    REQUIRE(emulator.registers()[4] == 0u);
}

TEST_CASE("emulator validates ASP instructions register fields") {
    SECTION("ASPI destination") {
        const std::vector<std::uint8_t> program = {
            0x22, 0x10, 0x00, 0x00 // ASPI R16 0
        };

        emulator::Emulator emulator;
        emulator.loadProgram(program);
        REQUIRE_THROWS_WITH(emulator.run(), ContainsSubstring("invalid register index in destination"));
    }

    SECTION("ASPR destination") {
        const std::vector<std::uint8_t> program = {
            0x23, 0x10, 0x00, 0x00 // ASPR R16 R0
        };

        emulator::Emulator emulator;
        emulator.loadProgram(program);
        REQUIRE_THROWS_WITH(emulator.run(), ContainsSubstring("invalid register index in destination"));
    }

    SECTION("ASPR source") {
        const std::vector<std::uint8_t> program = {
            0x23, 0x00, 0x10, 0x00 // ASPR R0 R16
        };

        emulator::Emulator emulator;
        emulator.loadProgram(program);
        REQUIRE_THROWS_WITH(emulator.run(), ContainsSubstring("invalid register index in source"));
    }
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

TEST_CASE("emulator validates register fields in memory-format instructions") {
    SECTION("load destination register") {
        const std::vector<std::uint8_t> program = {
            0x10, 0x10, 0x00, 0x00 // LDB R16 R0 0
        };

        emulator::Emulator emulator;
        emulator.loadProgram(program);

        REQUIRE_THROWS_WITH(
            emulator.run(),
            ContainsSubstring("invalid register index in destination")
        );
    }

    SECTION("load base register") {
        const std::vector<std::uint8_t> program = {
            0x10, 0x00, 0x10, 0x00 // LDB R0 R16 0
        };

        emulator::Emulator emulator;
        emulator.loadProgram(program);

        REQUIRE_THROWS_WITH(
            emulator.run(),
            ContainsSubstring("invalid register index in base address")
        );
    }

    SECTION("store source register") {
        const std::vector<std::uint8_t> program = {
            0x15, 0x10, 0x00, 0x00 // STW R16 R0 0
        };

        emulator::Emulator emulator;
        emulator.loadProgram(program);

        REQUIRE_THROWS_WITH(
            emulator.run(),
            ContainsSubstring("invalid register index in source")
        );
    }

    SECTION("immediate byte is not a register field") {
        const std::vector<std::uint8_t> program = {
            0x00, 0x00, 0x40, 0x00, // LI R0 64
            0x10, 0x01, 0x00, 0xFF  // LDB R1 R0 0xFF
        };

        emulator::Emulator emulator;
        emulator.loadProgram(program);

        REQUIRE_NOTHROW(emulator.run());
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
    emulator.enableDisasmTrace(trace);
    emulator.setTraceTicks({{1, 1}});
    emulator.run();

    REQUIRE(
        trace.str() ==
        "---- 1 0x4 ----\n"
        "0x01010102 ADD R1 (0xa) R1 (0x5) R1 (0x5)\n"
    );
}

TEST_CASE("trace reads aliased memory base register from pre-instruction state") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x40, 0x00, // LI R0 64
        0x10, 0x00, 0x00, 0x00  // LDB R0 R0 0
    };

    std::ostringstream trace;
    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.enableDisasmTrace(trace);
    emulator.setTraceTicks({{1, 1}});
    emulator.run();

    REQUIRE(
        trace.str() ==
        "---- 1 0x4 ----\n"
        "0x00000010 LDB R0 (0x0) R0 (0x40) imm (0x0)\n"
    );
}

TEST_CASE("trace prints stack instruction operands") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x05, 0x00, 0xC0, // LI R5 0xC000
        0x20, 0x01, 0x05, 0x00, // SCRW SP_TOP R5
        0x00, 0x05, 0x00, 0x40, // LI R5 0x4000
        0x20, 0x02, 0x05, 0x00, // SCRW SP_SIZE R5
        0x21, 0x06, 0x01, 0x00, // SCRR R6 SP_TOP
        0x22, 0x06, 0xF0, 0xFF, // ASPI R6 -16
        0x00, 0x05, 0xF0, 0xFF, // LI R5 0xFFF0
        0x01, 0x05, 0xFF, 0xFF, // LUI R5 0xFFFF
        0x00, 0x00, 0x00, 0xC0, // LI R0 0xC000
        0x20, 0x01, 0x00, 0x00, // SCRW SP_TOP R0
        0x23, 0x06, 0x05, 0x00  // ASPR R6 R5
    };

    std::ostringstream trace;
    emulator::Emulator emulator(65536);
    emulator.loadProgram(program);
    emulator.enableDisasmTrace(trace);
    emulator.setTraceTicks({{1, 1}, {4, 5}, {10, 10}});
    emulator.run();

    REQUIRE(
        trace.str() ==
        "---- 1 0x4 ----\n"
        "0x00050120 SCRW SP_TOP R5 (0xc000)\n"
        "---- 4 0x10 ----\n"
        "0x00010621 SCRR R6 (0xc000) SP_TOP (0xc000)\n"
        "---- 5 0x14 ----\n"
        "0xfff00622 ASPI R6 (0xbff0) imm (0xfff0)\n"
        "---- 10 0x28 ----\n"
        "0x00050623 ASPR R6 (0xbff0) R5 (0xfffffff0)\n"
    );
}

TEST_CASE("clear stack is reported as a RAM write by the releasing instruction") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x40, 0x00, // LI R0 0x40
        0x20, 0x01, 0x00, 0x00, // SCRW SP_TOP R0
        0x00, 0x00, 0x04, 0x00, // LI R0 4
        0x20, 0x02, 0x00, 0x00, // SCRW SP_SIZE R0
        0x22, 0x01, 0xFC, 0xFF, // ASPI R1 -4
        0x00, 0x02, 0xCD, 0xAB, // LI R2 0xABCD
        0x15, 0x02, 0x01, 0x00, // STW R2 R1 0
        0x22, 0x03, 0x04, 0x00  // ASPI R3 4
    };

    std::ostringstream trace;
    emulator::Emulator emulator;
    emulator.enableClearStack();
    emulator.loadProgram(program);
    emulator.setTraceTicks({{7, 7}});
    emulator.enableRamWriteTrace(trace);
    emulator.run();

    REQUIRE(
        trace.str() ==
        "7 RAM [0x0000003c] wr 0x0000abcd -> 0x00000000\n"
    );
}

TEST_CASE("emulator writes RAM trace for aligned STW") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x40, 0x00, // LI R0 64
        0x00, 0x01, 0x44, 0x33, // LI R1 0x3344
        0x01, 0x01, 0x22, 0x11, // LUI R1 0x1122
        0x15, 0x01, 0x00, 0x00  // STW R1 R0 0
    };

    std::ostringstream trace;
    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.enableRamWriteTrace(trace);
    emulator.run();

    REQUIRE(
        trace.str() ==
        "3 RAM [0x00000040] wr 0x00000000 -> 0x11223344\n"
    );
}

TEST_CASE("emulator writes RAM trace for unaligned STW touching two cells") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x40, 0x00, // LI R0 64
        0x00, 0x01, 0x44, 0x33, // LI R1 0x3344
        0x01, 0x01, 0x22, 0x11, // LUI R1 0x1122
        0x15, 0x01, 0x00, 0x01  // STW R1 R0 1
    };

    std::ostringstream trace;
    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.enableRamWriteTrace(trace);
    emulator.run();

    REQUIRE(
        trace.str() ==
        "3 RAM [0x00000040] wr 0x00000000 -> 0x22334400\n"
        "3 RAM [0x00000044] wr 0x00000000 -> 0x00000011\n"
    );
}

TEST_CASE("RAM trace address filter matches touched bytes inside a cell") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x40, 0x00, // LI R0 64
        0x00, 0x01, 0x44, 0x33, // LI R1 0x3344
        0x01, 0x01, 0x22, 0x11, // LUI R1 0x1122
        0x15, 0x01, 0x00, 0x01  // STW R1 R0 1
    };

    std::ostringstream trace;
    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.enableRamWriteTrace(trace, {{65, 65}});
    emulator.run();

    REQUIRE(
        trace.str() ==
        "3 RAM [0x00000040] wr 0x00000000 -> 0x22334400\n"
    );
}

TEST_CASE("RAM trace applies tick and address filters together") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x40, 0x00, // LI R0 64
        0x00, 0x01, 0x44, 0x33, // LI R1 0x3344
        0x01, 0x01, 0x22, 0x11, // LUI R1 0x1122
        0x15, 0x01, 0x00, 0x00, // STW R1 R0 0
        0x15, 0x01, 0x00, 0x04  // STW R1 R0 4
    };

    std::ostringstream trace;
    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.setTraceTicks({{4, 4}});
    emulator.enableRamWriteTrace(trace, {{68, 71}});
    emulator.run();

    REQUIRE(
        trace.str() ==
        "4 RAM [0x00000044] wr 0x00000000 -> 0x11223344\n"
    );
}

TEST_CASE("trace tick ranges are inclusive and overlapping ranges are merged") {
    emulator::TickRangeFilter filter;
    filter.setRanges(emulator::parseTickRanges("7-10,0,3-5,4-8"));

    REQUIRE(filter.contains(0));
    REQUIRE_FALSE(filter.contains(1));
    REQUIRE_FALSE(filter.contains(2));
    REQUIRE(filter.contains(3));
    REQUIRE(filter.contains(10));
    REQUIRE_FALSE(filter.contains(11));
}

TEST_CASE("trace tick range parser rejects malformed ranges") {
    REQUIRE_THROWS_AS(emulator::parseTickRanges(""), std::invalid_argument);
    REQUIRE_THROWS_AS(emulator::parseTickRanges("1,,2"), std::invalid_argument);
    REQUIRE_THROWS_AS(emulator::parseTickRanges("5-2"), std::invalid_argument);
    REQUIRE_THROWS_AS(emulator::parseTickRanges("1-x"), std::invalid_argument);
}

TEST_CASE("saves the parameters of the used stream") {
    const std::vector<std::uint8_t> program = {
        0x00, 0x00, 0x01, 0x00 // LI R0 1
    };

    std::ostringstream trace;
    trace << std::uppercase << std::oct;
    trace.fill('*');
    trace.width(17);

    const auto expectedFlags = trace.flags();
    const auto expectedFill = trace.fill();
    const auto expectedWidth = trace.width();

    emulator::Emulator emulator;
    emulator.loadProgram(program);
    emulator.enableDisasmTrace(trace);
    emulator.run();

    REQUIRE(
        trace.str() ==
        "---- 0 0x0 ----\n"
        "0x00010000 LI R0 (0x1) imm (0x1)\n"
    );
    REQUIRE(trace.flags() == expectedFlags);
    REQUIRE(trace.fill() == expectedFill);
    REQUIRE(trace.width() == expectedWidth);
}

#endif
