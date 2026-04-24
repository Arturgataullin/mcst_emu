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
        std::vector<Token> tokenize();
    private:
        bool isAtEnd() const noexcept;
        char current() const noexcept;
        char peek(std::size_t offset = 1) const noexcept;
        char advance() noexcept;
        void skipWhitespace() noexcept;
        void skipComment() noexcept;

        Token makeSimpleToken(TokenType type, std::string lexeme, SourceLocation location) const;
        Token lexOprationOrRegister();
        Token lexNumber();

        std::uint64_t parseInteger(const std::string& lexeme, const SourceLocation& location);

        void fail(const SourceLocation& location, const std::string& message) const;

        static std::string toUpperCopy(std::string_view text);
        static bool isOperationLike(const std::string& upperLexeme);
        static std::optional<std::int64_t> parseRegisterIndex(std::string_view lexeme);
        static bool isCommentStart(const char ch);
        static bool isTokenSeparator(char ch, char next_ch);
    };
}