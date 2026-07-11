#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace assembler {

enum class TokenType {
    Operation,
    Register,
    Number,
    NewLine,
    EndOfFile
};

struct SourceLocation {
    std::size_t line = 1;
    std::size_t column = 1;
};

struct Token {
    TokenType type {};
    std::string lexeme;
    SourceLocation location {};
    std::optional<std::uint64_t> numberValue {};
};

[[nodiscard]] std::string_view toString(TokenType type);

}
