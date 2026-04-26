#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "opMemory.h"

#include <cstdint>
#include <vector>

using Catch::Matchers::ContainsSubstring;

// weak test
TEST_CASE("memory is zeroed after clear") {
    emulator::Memory memory;

    memory.clear();

    REQUIRE(memory.read8(0) == 0x00);
    REQUIRE(memory.read8(1) == 0x00);
    REQUIRE(memory.read8(100) == 0x00);
    REQUIRE(memory.read8(common::opMemorySize - 1) == 0x00);
}

TEST_CASE("memory load stores bytes at base address") {
    emulator::Memory memory;
    memory.clear();

    const std::vector<std::uint8_t> bytes = {
        0x00, 0x01, 0x00, 0x80
    };

    memory.load(bytes, 16);

    REQUIRE(memory.read8(16) == 0x00);
    REQUIRE(memory.read8(17) == 0x01);
    REQUIRE(memory.read8(18) == 0x00);
    REQUIRE(memory.read8(19) == 0x80);
}

TEST_CASE("memory read32 reconstructs instruction word in expected byte order") {
    emulator::Memory memory;
    memory.clear();

    const std::vector<std::uint8_t> bytes = {
        0x00, 0x01, 0x00, 0x80
    };

    memory.load(bytes, 0);

    const std::uint32_t word = memory.read32(0);

    REQUIRE(word == 0x80000100u);
}

TEST_CASE("memory read32 works for ADD instruction bytes") {
    emulator::Memory memory;
    memory.clear();

    const std::vector<std::uint8_t> bytes = {
        0x02, 0x01, 0x01, 0x00
    };

    memory.load(bytes, 0);

    const std::uint32_t word = memory.read32(0);

    REQUIRE(word == 0x00010102u);
}

TEST_CASE("memory write8 overwrites byte") {
    emulator::Memory memory;
    memory.clear();

    memory.write8(42, 0xAB);

    REQUIRE(memory.read8(42) == 0xAB);
}

TEST_CASE("memory load does not affect bytes outside loaded range") {
    emulator::Memory memory;
    memory.clear();

    const std::vector<std::uint8_t> bytes = {
        0x11, 0x22, 0x33
    };

    memory.load(bytes, 10);

    REQUIRE(memory.read8(9) == 0x00);
    REQUIRE(memory.read8(10) == 0x11);
    REQUIRE(memory.read8(11) == 0x22);
    REQUIRE(memory.read8(12) == 0x33);
    REQUIRE(memory.read8(13) == 0x00);
}

TEST_CASE("memory throws on read8 out of range") {
    emulator::Memory memory;
    memory.clear();

    REQUIRE_THROWS_WITH(
        memory.read8(common::opMemorySize),
        ContainsSubstring("out of range")
    );
}

TEST_CASE("memory throws on write8 out of range") {
    emulator::Memory memory;
    memory.clear();

    REQUIRE_THROWS_WITH(
        memory.write8(common::opMemorySize, 0x12),
        ContainsSubstring("out of range")
    );
}

TEST_CASE("memory throws on read32 when range exceeds memory size") {
    emulator::Memory memory;
    memory.clear();

    REQUIRE_THROWS_WITH(
        memory.read32(common::opMemorySize - 2),
        ContainsSubstring("out of range")
    );
}

TEST_CASE("memory throws when load exceeds memory size") {
    emulator::Memory memory;
    memory.clear();

    const std::vector<std::uint8_t> bytes = {
        0xAA, 0xBB, 0xCC, 0xDD
    };

    REQUIRE_THROWS_WITH(
        memory.load(bytes, common::opMemorySize - 2),
        ContainsSubstring("out of range")
    );
}
