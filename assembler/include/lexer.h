#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "token.h"

namespace assembler {
class Lexer {

private:
    std::string source_;
    std::size_t pos_ = 0;
    std::size_t line_ = 1;
    std::size_t column_ = 1;

    static constexpr char comment = '#';

public:
    explicit Lexer(std::string_view src);
    [[nodiscard]] std::vector<Token> tokenize();
private:
    [[nodiscard]] bool isAtEnd() const noexcept;
    [[nodiscard]] char current() const noexcept;
    [[nodiscard]] char peek(std::size_t offset = 1) const noexcept;
    char advance() noexcept;
    void skipWhitespace() noexcept;
    void skipComment() noexcept;

    [[nodiscard]] Token makeSimpleToken(TokenType type, std::string lexeme, SourceLocation location) const;
    [[nodiscard]] Token lexWord();
    [[nodiscard]] Token lexNumber();

    [[nodiscard]] std::uint64_t parseInteger(const std::string& lexeme, const SourceLocation& location);

    [[noreturn]] void fail(const SourceLocation& location, const std::string& message) const;

    [[nodiscard]] static std::string toUpperCopy(std::string_view text);
    [[nodiscard]] static bool isOperationLike(const std::string& upperLexeme);
    [[nodiscard]] static std::optional<std::int64_t> parseRegisterIndex(std::string_view lexeme);
    [[nodiscard]] static bool isCommentStart(const char ch);
    [[nodiscard]] static bool isTokenSeparator(char ch, char next_ch);
};
}
