#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "lexer.h"

using namespace assembler;
using Catch::Matchers::ContainsSubstring;

TEST_CASE("lexer returns only EOF for empty input") {
    Lexer lexer("");
    const auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 1);
    CHECK(tokens[0].type == TokenType::EndOfFile);
}

TEST_CASE("lexer returns only EOF for whitespace-only input") {
    Lexer lexer("            ");
    const auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 1);
    CHECK(tokens[0].type == TokenType::EndOfFile);
}

TEST_CASE("lexer tokenizes a single newline") {
    Lexer lexer("\n");
    const auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].type == TokenType::NewLine);
    CHECK(tokens[1].type == TokenType::EndOfFile);
}

TEST_CASE("lexer doesnt tokenize comments") {
    Lexer lexer("ADD #R1 R2 R3\nLI");
    const auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 4);
    CHECK(tokens[0].type == TokenType::Operation);
    CHECK(tokens[1].type == TokenType::NewLine);
    CHECK(tokens[2].type == TokenType::Operation);
    CHECK(tokens[3].type == TokenType::EndOfFile);
}

TEST_CASE("lexer tokenizes LI instruction") {
    SECTION ("with NewLine on the end") {
        Lexer lexer("LI R0 0x1\n");
        const auto tokens = lexer.tokenize();

        REQUIRE(tokens.size() == 5);

        CHECK(tokens[0].type == TokenType::Operation);
        CHECK(tokens[0].lexeme == "LI");

        CHECK(tokens[1].type == TokenType::Register);
        CHECK(tokens[1].lexeme == "R0");

        CHECK(tokens[2].type == TokenType::Number);
        CHECK(tokens[2].lexeme == "0x1");
        REQUIRE(tokens[2].numberValue.has_value());
        CHECK(tokens[2].numberValue.value() == 1);

        CHECK(tokens[3].type == TokenType::NewLine);
        CHECK(tokens[4].type == TokenType::EndOfFile);
    }
    SECTION ("without NewLine on the end") {
        Lexer lexer("LI R0 0x1");
        const auto tokens = lexer.tokenize();

        REQUIRE(tokens.size() == 4);

        CHECK(tokens[0].type == TokenType::Operation);
        CHECK(tokens[0].lexeme == "LI");

        CHECK(tokens[1].type == TokenType::Register);
        CHECK(tokens[1].lexeme == "R0");

        CHECK(tokens[2].type == TokenType::Number);
        CHECK(tokens[2].lexeme == "0x1");
        REQUIRE(tokens[2].numberValue.has_value());
        CHECK(tokens[2].numberValue.value() == 1);

        CHECK(tokens[3].type == TokenType::EndOfFile);
    }
    SECTION ("with windows-styled NewLine") {
        Lexer lexer("LI R0 0x1\r\n");
        const auto tokens = lexer.tokenize();

        REQUIRE(tokens.size() == 5);
        CHECK(tokens[3].type == TokenType::NewLine);
        CHECK(tokens[4].type == TokenType::EndOfFile);
    }
    SECTION ("tab-separated instruction") {
        Lexer lexer("LI\tR0\t0x1");
        const auto tokens = lexer.tokenize();

        REQUIRE(tokens.size() == 4);
        CHECK(tokens[0].type == TokenType::Operation);
        CHECK(tokens[1].type == TokenType::Register);
        CHECK(tokens[2].type == TokenType::Number);
        CHECK(tokens[3].type == TokenType::EndOfFile);
    }
}

TEST_CASE("lexer tokenizes ADD instruction") {
    Lexer lexer("ADD R0 R1 R2\n");
    const auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 6);

    CHECK(tokens[0].type == TokenType::Operation);
    CHECK(tokens[0].lexeme == "ADD");

    CHECK(tokens[1].type == TokenType::Register);
    CHECK(tokens[1].lexeme == "R0");

    CHECK(tokens[2].type == TokenType::Register);
    CHECK(tokens[2].lexeme == "R1");

    CHECK(tokens[3].type == TokenType::Register);
    CHECK(tokens[3].lexeme == "R2");

    CHECK(tokens[4].type == TokenType::NewLine);
    CHECK(tokens[5].type == TokenType::EndOfFile);
}

TEST_CASE("lexer tokenizes LUI instruction") {
    Lexer lexer("LUI R0 0x1234\n");
    const auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 5);

    CHECK(tokens[0].type == TokenType::Operation);
    CHECK(tokens[0].lexeme == "LUI");

    CHECK(tokens[1].type == TokenType::Register);
    CHECK(tokens[1].lexeme == "R0");

    CHECK(tokens[2].type == TokenType::Number);
    CHECK(tokens[2].lexeme == "0x1234");
    REQUIRE(tokens[2].numberValue.has_value());
    CHECK(tokens[2].numberValue.value() == 0x1234);

    CHECK(tokens[3].type == TokenType::NewLine);
    CHECK(tokens[4].type == TokenType::EndOfFile);
}

TEST_CASE("lexer tokenizes SUB instruction") {
    Lexer lexer("SUB R0 R1 R2\n");
    const auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 6);

    CHECK(tokens[0].type == TokenType::Operation);
    CHECK(tokens[0].lexeme == "SUB");

    CHECK(tokens[1].type == TokenType::Register);
    CHECK(tokens[1].lexeme == "R0");

    CHECK(tokens[2].type == TokenType::Register);
    CHECK(tokens[2].lexeme == "R1");

    CHECK(tokens[3].type == TokenType::Register);
    CHECK(tokens[3].lexeme == "R2");

    CHECK(tokens[4].type == TokenType::NewLine);
    CHECK(tokens[5].type == TokenType::EndOfFile);
}

TEST_CASE("lexer tokenizes AND instruction") {
    Lexer lexer("AND R0 R1 R2\n");
    const auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 6);

    CHECK(tokens[0].type == TokenType::Operation);
    CHECK(tokens[0].lexeme == "AND");

    CHECK(tokens[1].type == TokenType::Register);
    CHECK(tokens[1].lexeme == "R0");

    CHECK(tokens[2].type == TokenType::Register);
    CHECK(tokens[2].lexeme == "R1");

    CHECK(tokens[3].type == TokenType::Register);
    CHECK(tokens[3].lexeme == "R2");

    CHECK(tokens[4].type == TokenType::NewLine);
    CHECK(tokens[5].type == TokenType::EndOfFile);
}

TEST_CASE("lexer tokenizes OR instruction") {
    Lexer lexer("OR R0 R1 R2\n");
    const auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 6);

    CHECK(tokens[0].type == TokenType::Operation);
    CHECK(tokens[0].lexeme == "OR");

    CHECK(tokens[1].type == TokenType::Register);
    CHECK(tokens[1].lexeme == "R0");

    CHECK(tokens[2].type == TokenType::Register);
    CHECK(tokens[2].lexeme == "R1");

    CHECK(tokens[3].type == TokenType::Register);
    CHECK(tokens[3].lexeme == "R2");

    CHECK(tokens[4].type == TokenType::NewLine);
    CHECK(tokens[5].type == TokenType::EndOfFile);
}

TEST_CASE("lexer tokenizes XOR instruction") {
    Lexer lexer("XOR R0 R1 R2\n");
    const auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 6);

    CHECK(tokens[0].type == TokenType::Operation);
    CHECK(tokens[0].lexeme == "XOR");

    CHECK(tokens[1].type == TokenType::Register);
    CHECK(tokens[1].lexeme == "R0");

    CHECK(tokens[2].type == TokenType::Register);
    CHECK(tokens[2].lexeme == "R1");

    CHECK(tokens[3].type == TokenType::Register);
    CHECK(tokens[3].lexeme == "R2");

    CHECK(tokens[4].type == TokenType::NewLine);
    CHECK(tokens[5].type == TokenType::EndOfFile);
}

TEST_CASE("lexer tokenizes SLL instruction") {
    Lexer lexer("SLL R0 R1 R2\n");
    const auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 6);

    CHECK(tokens[0].type == TokenType::Operation);
    CHECK(tokens[0].lexeme == "SLL");

    CHECK(tokens[1].type == TokenType::Register);
    CHECK(tokens[1].lexeme == "R0");

    CHECK(tokens[2].type == TokenType::Register);
    CHECK(tokens[2].lexeme == "R1");

    CHECK(tokens[3].type == TokenType::Register);
    CHECK(tokens[3].lexeme == "R2");

    CHECK(tokens[4].type == TokenType::NewLine);
    CHECK(tokens[5].type == TokenType::EndOfFile);
}

TEST_CASE("lexer tokenizes SRL instruction") {
    Lexer lexer("SRL R0 R1 R2\n");
    const auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 6);

    CHECK(tokens[0].type == TokenType::Operation);
    CHECK(tokens[0].lexeme == "SRL");

    CHECK(tokens[1].type == TokenType::Register);
    CHECK(tokens[1].lexeme == "R0");

    CHECK(tokens[2].type == TokenType::Register);
    CHECK(tokens[2].lexeme == "R1");

    CHECK(tokens[3].type == TokenType::Register);
    CHECK(tokens[3].lexeme == "R2");

    CHECK(tokens[4].type == TokenType::NewLine);
    CHECK(tokens[5].type == TokenType::EndOfFile);
}

TEST_CASE("lexer tokenizes SRA instruction") {
    Lexer lexer("SRA R0 R1 R2\n");
    const auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 6);

    CHECK(tokens[0].type == TokenType::Operation);
    CHECK(tokens[0].lexeme == "SRA");

    CHECK(tokens[1].type == TokenType::Register);
    CHECK(tokens[1].lexeme == "R0");

    CHECK(tokens[2].type == TokenType::Register);
    CHECK(tokens[2].lexeme == "R1");

    CHECK(tokens[3].type == TokenType::Register);
    CHECK(tokens[3].lexeme == "R2");

    CHECK(tokens[4].type == TokenType::NewLine);
    CHECK(tokens[5].type == TokenType::EndOfFile);
}

TEST_CASE("lexer tokenizes MUL instruction") {
    Lexer lexer("MUL R0 R1 R2\n");
    const auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 6);

    CHECK(tokens[0].type == TokenType::Operation);
    CHECK(tokens[0].lexeme == "MUL");

    CHECK(tokens[1].type == TokenType::Register);
    CHECK(tokens[1].lexeme == "R0");

    CHECK(tokens[2].type == TokenType::Register);
    CHECK(tokens[2].lexeme == "R1");

    CHECK(tokens[3].type == TokenType::Register);
    CHECK(tokens[3].lexeme == "R2");

    CHECK(tokens[4].type == TokenType::NewLine);
    CHECK(tokens[5].type == TokenType::EndOfFile);
}

TEST_CASE("lexer tokenizes UDIV instruction") {
    Lexer lexer("UDIV R0 R1 R2\n");
    const auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 6);

    CHECK(tokens[0].type == TokenType::Operation);
    CHECK(tokens[0].lexeme == "UDIV");

    CHECK(tokens[1].type == TokenType::Register);
    CHECK(tokens[1].lexeme == "R0");

    CHECK(tokens[2].type == TokenType::Register);
    CHECK(tokens[2].lexeme == "R1");

    CHECK(tokens[3].type == TokenType::Register);
    CHECK(tokens[3].lexeme == "R2");

    CHECK(tokens[4].type == TokenType::NewLine);
    CHECK(tokens[5].type == TokenType::EndOfFile);
}

TEST_CASE("lexer tokenizes SDIV instruction") {
    Lexer lexer("SDIV R0 R1 R2\n");
    const auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 6);

    CHECK(tokens[0].type == TokenType::Operation);
    CHECK(tokens[0].lexeme == "SDIV");

    CHECK(tokens[1].type == TokenType::Register);
    CHECK(tokens[1].lexeme == "R0");

    CHECK(tokens[2].type == TokenType::Register);
    CHECK(tokens[2].lexeme == "R1");

    CHECK(tokens[3].type == TokenType::Register);
    CHECK(tokens[3].lexeme == "R2");

    CHECK(tokens[4].type == TokenType::NewLine);
    CHECK(tokens[5].type == TokenType::EndOfFile);
}

TEST_CASE("lexer tokenizes MOV instruction") {
    Lexer lexer("MOV R0 R1\n");
    const auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 5);

    CHECK(tokens[0].type == TokenType::Operation);
    CHECK(tokens[0].lexeme == "MOV");

    CHECK(tokens[1].type == TokenType::Register);
    CHECK(tokens[1].lexeme == "R0");

    CHECK(tokens[2].type == TokenType::Register);
    CHECK(tokens[2].lexeme == "R1");

    CHECK(tokens[3].type == TokenType::NewLine);
    CHECK(tokens[4].type == TokenType::EndOfFile);
}

TEST_CASE("lexer tokenizes NEG instruction") {
    Lexer lexer("NEG R0 R1\n");
    const auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 5);

    CHECK(tokens[0].type == TokenType::Operation);
    CHECK(tokens[0].lexeme == "NEG");

    CHECK(tokens[1].type == TokenType::Register);
    CHECK(tokens[1].lexeme == "R0");

    CHECK(tokens[2].type == TokenType::Register);
    CHECK(tokens[2].lexeme == "R1");

    CHECK(tokens[3].type == TokenType::NewLine);
    CHECK(tokens[4].type == TokenType::EndOfFile);
}

TEST_CASE("lexer tokenizes NOT instruction") {
    Lexer lexer("NOT R0\n");
    const auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 4);

    CHECK(tokens[0].type == TokenType::Operation);
    CHECK(tokens[0].lexeme == "NOT");

    CHECK(tokens[1].type == TokenType::Register);
    CHECK(tokens[1].lexeme == "R0");

    CHECK(tokens[2].type == TokenType::NewLine);
    CHECK(tokens[3].type == TokenType::EndOfFile);
}

TEST_CASE("lexer tokenizes LFI instruction") {
    Lexer lexer("LFI R0 0x12345678\n");
    const auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 5);

    CHECK(tokens[0].type == TokenType::Operation);
    CHECK(tokens[0].lexeme == "LFI");

    CHECK(tokens[1].type == TokenType::Register);
    CHECK(tokens[1].lexeme == "R0");

    CHECK(tokens[2].type == TokenType::Number);
    CHECK(tokens[2].lexeme == "0x12345678");
    REQUIRE(tokens[2].numberValue.has_value());
    CHECK(tokens[2].numberValue.value() == 0x12345678);

    CHECK(tokens[3].type == TokenType::NewLine);
    CHECK(tokens[4].type == TokenType::EndOfFile);
}

TEST_CASE("lexer tokenizes multiple instructions") {
    Lexer lexer("ADD R0 R1 R2\nLI R0 0x1\n");
    const auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 10);

    CHECK(tokens[0].type == TokenType::Operation);
    CHECK(tokens[0].lexeme == "ADD");

    CHECK(tokens[1].type == TokenType::Register);
    CHECK(tokens[1].lexeme == "R0");

    CHECK(tokens[2].type == TokenType::Register);
    CHECK(tokens[2].lexeme == "R1");

    CHECK(tokens[3].type == TokenType::Register);
    CHECK(tokens[3].lexeme == "R2");

    CHECK(tokens[4].type == TokenType::NewLine);

    CHECK(tokens[5].type == TokenType::Operation);
    CHECK(tokens[5].lexeme == "LI");

    CHECK(tokens[6].type == TokenType::Register);
    CHECK(tokens[6].lexeme == "R0");

    CHECK(tokens[7].type == TokenType::Number);
    CHECK(tokens[7].lexeme == "0x1");
    REQUIRE(tokens[7].numberValue.has_value());
    CHECK(tokens[7].numberValue.value() == 1);

    CHECK(tokens[8].type == TokenType::NewLine);
    CHECK(tokens[9].type == TokenType::EndOfFile);
}

TEST_CASE("lexer tokenizes status registers") {
    Lexer lexer("IP SP_TOP SP_SIZE");
    const auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 4);

    CHECK(tokens[0].type == TokenType::StatusRegister);
    CHECK(tokens[0].lexeme == "IP");
    REQUIRE(tokens[0].numberValue.has_value());
    CHECK(tokens[0].numberValue.value() == 0);

    CHECK(tokens[1].type == TokenType::StatusRegister);
    CHECK(tokens[1].lexeme == "SP_TOP");
    REQUIRE(tokens[1].numberValue.has_value());
    CHECK(tokens[1].numberValue.value() == 1);

    CHECK(tokens[2].type == TokenType::StatusRegister);
    CHECK(tokens[2].lexeme == "SP_SIZE");
    REQUIRE(tokens[2].numberValue.has_value());
    CHECK(tokens[2].numberValue.value() == 2);

    CHECK(tokens[3].type == TokenType::EndOfFile);
}

TEST_CASE("lexer rejects unknown stack status register") {
    Lexer lexer("SP_BAD");

    REQUIRE_THROWS_WITH(
        lexer.tokenize(),
        ContainsSubstring("unknown word 'SP_BAD'")
    );
}

TEST_CASE("lexer still tokenizes all general-purpose registers") {
    Lexer lexer("R0 R1 R2 R3 R4 R5 R6 R7 R8 R9 R10 R11 R12 R13 R14 R15");
    const auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 17);
    for (std::size_t i = 0; i < 16; ++i) {
        CHECK(tokens[i].type == TokenType::Register);
        REQUIRE(tokens[i].numberValue.has_value());
        CHECK(tokens[i].numberValue.value() == i);
    }
    CHECK(tokens[16].type == TokenType::EndOfFile);
}

TEST_CASE("lexer rejects invalid numeric literals") {
    SECTION ("non-hex character after hex prefix") {
        Lexer lexer("0x1VZ");
        REQUIRE_THROWS_AS(lexer.tokenize(), std::runtime_error);
    }

    SECTION ("decimal literal is not allowed") {
        Lexer lexer("24");
        REQUIRE_THROWS_AS(lexer.tokenize(), std::runtime_error);
    }

    SECTION ("hex literal does not fit into uint64") {
        Lexer lexer("0x222222222222222222222222222222222222222222222");
        REQUIRE_THROWS_AS(lexer.tokenize(), std::runtime_error);
    }
}

TEST_CASE("lexer rejects unknown words") {
    SECTION ("unknown word-like token") {
        Lexer lexer("ab aba aa");
        REQUIRE_THROWS_AS(lexer.tokenize(), std::runtime_error);
    }
}
