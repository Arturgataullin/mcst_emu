#include <catch2/catch_test_macros.hpp>

#include "../../../assembler/include/lexer.h"

using namespace assembler;

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