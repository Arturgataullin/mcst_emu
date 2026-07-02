#include <catch2/catch_test_macros.hpp>

#include "assembler.h"
#include "emulator.h"
#include "../support/temporary_directory.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace {

using test_support::TemporaryDirectory;

void writeSource(const fs::path& path, const std::string& source) {
    std::ofstream out(path, std::ios::binary);
    REQUIRE(out.good());

    out << source;
    REQUIRE(out.good());
}

std::array<std::uint32_t, common::registerCount> assembleAndRun(
    const std::string& testName,
    const std::string& source
) {
    TemporaryDirectory temp("assembler_emulator_" + testName);
    const fs::path sourcePath = temp.path() / "program.s";
    const fs::path binaryPath = temp.path() / "program.o";

    writeSource(sourcePath, source);

    assembler::Assembler assembler;
    assembler.assembleFile(sourcePath.string(), binaryPath.string());

    emulator::Emulator emulator;
    emulator.loadProgramFromFile(binaryPath.string());
    emulator.run();

    return emulator.registers();
}

}

TEST_CASE("assembled pseudo instruction program executes correctly") {
    const auto registers = assembleAndRun(
        "pseudo_program",
        "LFI R0 0x12345678\n"
        "MOV R1 R0\n"
        "NEG R2 R1\n"
        "NOT R2\n"
    );

    REQUIRE(registers[0] == 0x12345678u);
    REQUIRE(registers[1] == 0x12345678u);
    REQUIRE(registers[2] == 0x12345677u);
}

TEST_CASE("assembled NEG works when source and destination are equal") {
    const auto registers = assembleAndRun(
        "neg_alias",
        "LFI R1 0x00000005\n"
        "NEG R1 R1\n"
    );

    REQUIRE(registers[1] == 0xFFFFFFFBu);
}

TEST_CASE("assembled NOT works for highest user register") {
    const auto registers = assembleAndRun(
        "not_r15",
        "LFI R15 0x000000FF\n"
        "NOT R15\n"
    );

    REQUIRE(registers[15] == 0xFFFFFF00u);
}
