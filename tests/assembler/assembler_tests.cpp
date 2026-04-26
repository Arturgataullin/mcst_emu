#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "assembler.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using Catch::Matchers::ContainsSubstring;

namespace fs = std::filesystem;

namespace {

std::string readTextFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    REQUIRE(in.good());

    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

std::vector<std::uint8_t> readBinaryFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    REQUIRE(in.good());

    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void writeTextFile(const fs::path& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary);
    REQUIRE(out.good());

    out << content;
    REQUIRE(out.good());
}

fs::path makeTempDir(const std::string& nameSuffix) {
    const fs::path dir =
        fs::temp_directory_path() /
        fs::path("assembler_tests_" + nameSuffix);

    fs::remove_all(dir);
    fs::create_directories(dir);
    return dir;
}

}

TEST_CASE("assembler assembles sample program into expected bytes") {
    const fs::path tempDir = makeTempDir("assemble_sample");

    const fs::path inputPath = tempDir / "in.s";
    const fs::path outputPath = tempDir / "out.o";

    const std::string source =
        "LI R0 0x1\n"
        "LI R1 0x8000\n"
        "ADD R1 R1 R0\n";

    writeTextFile(inputPath, source);

    assembler::Assembler assembler;
    assembler.assembleFile(inputPath.string(), outputPath.string());

    REQUIRE(fs::exists(outputPath));

    const std::vector<std::uint8_t> bytes = readBinaryFile(outputPath);

    const std::vector<std::uint8_t> expected = {
        0x00, 0x00, 0x01, 0x00,
        0x00, 0x01, 0x00, 0x80,
        0x02, 0x01, 0x01, 0x00
    };

    REQUIRE(bytes == expected);

    fs::remove_all(tempDir);
}

TEST_CASE("assembler assembles single LI instruction") {
    const fs::path tempDir = makeTempDir("assemble_single_li");

    const fs::path inputPath = tempDir / "in.s";
    const fs::path outputPath = tempDir / "out.o";

    writeTextFile(inputPath, "LI R3 0x1234\n");

    assembler::Assembler assembler;
    assembler.assembleFile(inputPath.string(), outputPath.string());

    const std::vector<std::uint8_t> bytes = readBinaryFile(outputPath);

    const std::vector<std::uint8_t> expected = {
        0x00, 0x03, 0x34, 0x12
    };

    REQUIRE(bytes == expected);

    fs::remove_all(tempDir);
}

TEST_CASE("assembler handles empty lines between instructions") {
    const fs::path tempDir = makeTempDir("assemble_empty_lines");

    const fs::path inputPath = tempDir / "in.s";
    const fs::path outputPath = tempDir / "out.o";

    const std::string source =
        "\n"
        "LI R0 0x1\n"
        "\n"
        "\n"
        "ADD R1 R0 R0\n"
        "\n";

    writeTextFile(inputPath, source);

    assembler::Assembler assembler;
    assembler.assembleFile(inputPath.string(), outputPath.string());

    const std::vector<std::uint8_t> bytes = readBinaryFile(outputPath);

    const std::vector<std::uint8_t> expected = {
        0x00, 0x00, 0x01, 0x00,
        0x02, 0x01, 0x00, 0x00
    };

    REQUIRE(bytes == expected);

    fs::remove_all(tempDir);
}

TEST_CASE("assembler throws when input file does not exist") {
    const fs::path tempDir = makeTempDir("assembler_missing_input");

    const fs::path inputPath = tempDir / "missing.s";
    const fs::path outputPath = tempDir / "out.o";

    assembler::Assembler assembler;

    REQUIRE_THROWS_WITH(
        assembler.assembleFile(inputPath.string(), outputPath.string()),
        ContainsSubstring("failed to open input file")
    );

    fs::remove_all(tempDir);
}

TEST_CASE("assembler throws on out-of-range register") {
    const fs::path tempDir = makeTempDir("assembler_bad_register");

    const fs::path inputPath = tempDir / "bad_reg.s";
    const fs::path outputPath = tempDir / "out.o";

    writeTextFile(inputPath, "LI R16 0x1\n");

    assembler::Assembler assembler;

    REQUIRE_THROWS_WITH(
        assembler.assembleFile(inputPath.string(), outputPath.string()),
        ContainsSubstring("register number must be from 0 to 15")
    );

    fs::remove_all(tempDir);
}

TEST_CASE("assembler throws on too-large immediate") {
    const fs::path tempDir = makeTempDir("assembler_bad_imm");

    const fs::path inputPath = tempDir / "bad_imm.s";
    const fs::path outputPath = tempDir / "out.o";

    writeTextFile(inputPath, "LI R0 0x10000\n");

    assembler::Assembler assembler;

    REQUIRE_THROWS_WITH(
        assembler.assembleFile(inputPath.string(), outputPath.string()),
        ContainsSubstring("immediate value must be in range")
    );

    fs::remove_all(tempDir);
}

TEST_CASE("assembler assembles empty file into empty output") {
    const fs::path tempDir = makeTempDir("assemble_empty_file");

    const fs::path inputPath = tempDir / "empty.s";
    const fs::path outputPath = tempDir / "out.o";

    writeTextFile(inputPath, "");

    assembler::Assembler assembler;
    assembler.assembleFile(inputPath.string(), outputPath.string());

    REQUIRE(fs::exists(outputPath));

    const std::vector<std::uint8_t> bytes = readBinaryFile(outputPath);
    REQUIRE(bytes.empty());

    fs::remove_all(tempDir);
}

TEST_CASE("assembler throws on wrong syntax") {
    
    const fs::path tempDir = makeTempDir("assembler_wrong_syntax");

    SECTION ("case 1") {
        const fs::path inputPath = tempDir / "wrong_syntax1.s";
        const fs::path outputPath = tempDir / "out1.o";

        writeTextFile(inputPath, "LI R0 0x1000 R12\n");

        assembler::Assembler assembler;

        REQUIRE_THROWS_WITH(
            assembler.assembleFile(inputPath.string(), outputPath.string()),
            ContainsSubstring("expected end of line after instruction")
        );
    }

    SECTION ("case 2") {
        const fs::path inputPath = tempDir / "wrong_syntax2.s";
        const fs::path outputPath = tempDir / "out2.o";

        writeTextFile(inputPath, "MOV R0 0x1000\n");

        assembler::Assembler assembler;

        REQUIRE_THROWS_WITH(
            assembler.assembleFile(inputPath.string(), outputPath.string()),
            ContainsSubstring("unknown word 'MOV'")
        );
    }

    SECTION ("case 3") {
        const fs::path inputPath = tempDir / "wrong_syntax3.s";
        const fs::path outputPath = tempDir / "out3.o";

        writeTextFile(inputPath, "ADD LOL R1 R2\n");

        assembler::Assembler assembler;

        REQUIRE_THROWS_WITH(
            assembler.assembleFile(inputPath.string(), outputPath.string()),
            ContainsSubstring("unknown word 'LOL'")
        );
    }

    SECTION ("case 4") {
        const fs::path inputPath = tempDir / "wrong_syntax4.s";
        const fs::path outputPath = tempDir / "out4.o";

        writeTextFile(inputPath, "LI 0x1 R0\n");

        assembler::Assembler assembler;

        REQUIRE_THROWS_WITH(
            assembler.assembleFile(inputPath.string(), outputPath.string()),
            ContainsSubstring("expected register operand")
        );

        fs::remove_all(tempDir);
    }

    
    fs::remove_all(tempDir);
}