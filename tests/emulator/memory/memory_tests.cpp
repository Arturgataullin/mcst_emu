#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "opMemory.h"

#include <cstdint>
#include <vector>

using Catch::Matchers::ContainsSubstring;

namespace {

struct CellWriteEvent {
    std::uint32_t address = 0;
    std::uint32_t oldData = 0;
    std::uint32_t newData = 0;
};

}

// weak test
TEST_CASE("memory is zeroed after clear") {
    emulator::Memory memory;

    memory.clear();

    REQUIRE(memory.read8(0) == 0x00);
    REQUIRE(memory.read8(1) == 0x00);
    REQUIRE(memory.read8(100) == 0x00);
    REQUIRE(memory.read8(common::opMemorySize - 1) == 0x00);
}

TEST_CASE("memory has configurable byte size") {
    emulator::Memory memory(16);

    REQUIRE(memory.size() == 16);
    REQUIRE(memory.read8(15) == 0x00);
    REQUIRE_THROWS_WITH(
        memory.read8(16),
        ContainsSubstring("out of range")
    );
}

TEST_CASE("memory rejects invalid sizes") {
    REQUIRE_THROWS_WITH(
        emulator::Memory(0),
        ContainsSubstring("positive")
    );

    REQUIRE_THROWS_WITH(
        emulator::Memory(3),
        ContainsSubstring("multiple of 4")
    );
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

TEST_CASE("memory write16 stores halfword in little-endian order") {
    emulator::Memory memory;
    memory.clear();

    memory.write16(3, 0xABCD);

    REQUIRE(memory.read8(3) == 0xCD);
    REQUIRE(memory.read8(4) == 0xAB);
    REQUIRE(memory.read16(3) == 0xABCD);
}

TEST_CASE("memory write32 stores word in little-endian order and supports unaligned access") {
    emulator::Memory memory;
    memory.clear();

    memory.write32(1, 0x11223344);

    REQUIRE(memory.read8(1) == 0x44);
    REQUIRE(memory.read8(2) == 0x33);
    REQUIRE(memory.read8(3) == 0x22);
    REQUIRE(memory.read8(4) == 0x11);
    REQUIRE(memory.read32(1) == 0x11223344);
}

TEST_CASE("memory reports cell write events once per affected cell") {
    emulator::Memory memory;
    std::vector<CellWriteEvent> events;

    memory.setCellWriteHandler([&events](
        std::uint32_t address,
        std::uint32_t oldData,
        std::uint32_t newData
    ) {
        events.push_back({address, oldData, newData});
    });

    memory.write32(1, 0x11223344);

    REQUIRE(events.size() == 2);
    REQUIRE(events[0].address == 0);
    REQUIRE(events[0].oldData == 0x00000000);
    REQUIRE(events[0].newData == 0x22334400);
    REQUIRE(events[1].address == 4);
    REQUIRE(events[1].oldData == 0x00000000);
    REQUIRE(events[1].newData == 0x00000011);
}

TEST_CASE("memory load does not report cell write events") {
    emulator::Memory memory;
    std::vector<CellWriteEvent> events;

    memory.setCellWriteHandler([&events](
        std::uint32_t address,
        std::uint32_t oldData,
        std::uint32_t newData
    ) {
        events.push_back({address, oldData, newData});
    });

    memory.load({0x11, 0x22, 0x33, 0x44}, 0);

    REQUIRE(events.empty());
    REQUIRE(memory.read32(0) == 0x44332211);
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

TEST_CASE("memory throws on write32 when range exceeds memory size without partial write") {
    emulator::Memory memory(4);

    memory.write32(0, 0x11223344);

    REQUIRE_THROWS_WITH(
        memory.write32(2, 0xAABBCCDD),
        ContainsSubstring("out of range")
    );

    REQUIRE(memory.read32(0) == 0x11223344);
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
