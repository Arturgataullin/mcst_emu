#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "assembler.h"
#include "isa.h"

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

TEST_CASE("assembler encodes all real register instructions") {
    const fs::path tempDir = makeTempDir("assemble_real_operations");

    const fs::path inputPath = tempDir / "in.s";
    const fs::path outputPath = tempDir / "out.o";

    const std::string source =
        "LUI R1 0x1234\n"
        "SUB R2 R3 R4\n"
        "AND R3 R4 R5\n"
        "OR R4 R5 R6\n"
        "XOR R5 R6 R7\n"
        "SLL R6 R7 R8\n"
        "SRL R7 R8 R9\n"
        "SRA R8 R9 R10\n"
        "MUL R9 R10 R11\n"
        "UDIV R10 R11 R12\n"
        "SDIV R11 R12 R13\n";

    writeTextFile(inputPath, source);

    assembler::Assembler assembler;
    assembler.assembleFile(inputPath.string(), outputPath.string());

    const std::vector<std::uint8_t> expected = {
        0x01, 0x01, 0x34, 0x12,
        0x03, 0x02, 0x03, 0x04,
        0x04, 0x03, 0x04, 0x05,
        0x05, 0x04, 0x05, 0x06,
        0x06, 0x05, 0x06, 0x07,
        0x07, 0x06, 0x07, 0x08,
        0x08, 0x07, 0x08, 0x09,
        0x09, 0x08, 0x09, 0x0A,
        0x0A, 0x09, 0x0A, 0x0B,
        0x0B, 0x0A, 0x0B, 0x0C,
        0x0C, 0x0B, 0x0C, 0x0D
    };

    REQUIRE(readBinaryFile(outputPath) == expected);

    fs::remove_all(tempDir);
}

TEST_CASE("assembler lowers pseudo instructions into expected bytes") {
    const fs::path tempDir = makeTempDir("assemble_pseudo_operations");

    const fs::path inputPath = tempDir / "in.s";
    const fs::path outputPath = tempDir / "out.o";

    const std::string source =
        "MOV R1 R2\n"
        "NEG R3 R4\n"
        "NOT R5\n"
        "LFI R6 0x12345678\n";

    writeTextFile(inputPath, source);

    assembler::Assembler assembler;
    assembler.assembleFile(inputPath.string(), outputPath.string());

    const std::uint8_t temp = common::assemblerTempRegister;
    const std::vector<std::uint8_t> expected = {
        0x05, 0x01, 0x02, 0x02,       // MOV -> OR R1 R2 R2
        0x00, temp, 0x00, 0x00,       // NEG -> LI temp 0
        0x03, 0x03, temp, 0x04,       //        SUB R3 temp R4
        0x00, temp, 0xFF, 0xFF,       // NOT -> LI temp 0xFFFF
        0x01, temp, 0xFF, 0xFF,       //        LUI temp 0xFFFF
        0x06, 0x05, 0x05, temp,       //        XOR R5 R5 temp
        0x00, 0x06, 0x78, 0x56,       // LFI -> LI R6 0x5678
        0x01, 0x06, 0x34, 0x12        //        LUI R6 0x1234
    };

    REQUIRE(readBinaryFile(outputPath) == expected);

    fs::remove_all(tempDir);
}

TEST_CASE("assembler accepts boundary values for LFI") {
    const fs::path tempDir = makeTempDir("assemble_lfi_boundaries");

    const fs::path inputPath = tempDir / "in.s";
    const fs::path outputPath = tempDir / "out.o";

    writeTextFile(
        inputPath,
        "LFI R0 0x0\n"
        "LFI R1 0xFFFFFFFF\n"
    );

    assembler::Assembler assembler;
    assembler.assembleFile(inputPath.string(), outputPath.string());

    const std::vector<std::uint8_t> expected = {
        0x00, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x00, 0x01, 0xFF, 0xFF,
        0x01, 0x01, 0xFF, 0xFF
    };

    REQUIRE(readBinaryFile(outputPath) == expected);

    fs::remove_all(tempDir);
}

TEST_CASE("assembler rejects LFI immediate larger than 32 bits") {
    const fs::path tempDir = makeTempDir("assembler_bad_lfi_imm");

    const fs::path inputPath = tempDir / "bad_lfi.s";
    const fs::path outputPath = tempDir / "out.o";

    writeTextFile(inputPath, "LFI R0 0x100000000\n");

    assembler::Assembler assembler;

    REQUIRE_THROWS_WITH(
        assembler.assembleFile(inputPath.string(), outputPath.string()),
        ContainsSubstring("immediate value is out of range")
    );

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
        ContainsSubstring("immediate value is out of range")
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
            ContainsSubstring("expected register operand")
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
