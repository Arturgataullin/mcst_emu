#include <algorithm>
#include <cctype>
#include <iostream>
#include <limits>
#include <sstream>
#include <unordered_set>

#include "lexer.h"
#include "isa.h"

namespace assembler {


namespace {

bool isHexDigit(char ch) noexcept {
    return std::isxdigit(static_cast<unsigned char>(ch)) != 0;
}

std::string formatErrorMessage(const SourceLocation& location, const std::string& message) {
    std::ostringstream out;
    out << "lexer error at line " << location.line
        << ", column " << location.column
        << ": " << message;
    return out.str();
}

bool isHexNumberStart(const char ch, const char next_ch) {
    return ch == '0' && (next_ch == 'x' || next_ch == 'X');
}

bool isWordStart(const char ch) {
    return std::isalpha(static_cast<unsigned char>(ch));
}

bool isNewLine(const char ch, const char next_ch) {
    return (ch == '\n' || ch == '\r' && next_ch == '\n');
}

}

Lexer::Lexer(std::string_view source) : source_(source) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (!isAtEnd()) {
        skipWhitespace();
        if (isAtEnd()) {
            break;
        }

        const SourceLocation location{line_, column_};
        const char ch = current();
        const char next_ch = peek();

        if (isCommentStart(ch)) {
            skipComment();
        }
        else if (isHexNumberStart(ch, next_ch))  {
            tokens.push_back(lexNumber());
        } 
        else if (isWordStart(ch)) {
            tokens.push_back(lexOprationOrRegister());
        } 
        else if (isNewLine(ch, next_ch)) {
            advance();
            tokens.push_back(makeSimpleToken(TokenType::NewLine, "", location));
        }
        else {
            fail(location, std::string("unexpected character '") + ch + "'");
        }
    }
    tokens.push_back(makeSimpleToken(TokenType::EndOfFile, "", SourceLocation{line_, column_}));

    return tokens;
}

Token Lexer::lexOprationOrRegister() {
    const SourceLocation location{line_, column_};
    const std::size_t start = pos_;

    while (!isTokenSeparator(current(), peek())) {
        advance();
    }
    std::string lexeme = source_.substr(start, pos_ - start);
    std::string upperLexeme = toUpperCopy(lexeme);

    // the register number will be saved in the numberValue field in the register token
    if (auto regIndex = parseRegisterIndex(upperLexeme)) {
        return Token{
            .type = TokenType::Register,
            .lexeme = upperLexeme,
            .location = location,
            .numberValue = *regIndex
        };
    }
    else if (isOperationLike(upperLexeme)) {
        return Token{
            .type = TokenType::Operation,
            .lexeme = upperLexeme,
            .location = location,
            .numberValue = std::nullopt
        };
    }
    
    fail(location, "unknown word '" + lexeme + "'");
}

Token Lexer::lexNumber() {
    const SourceLocation location{line_, column_};
    const std::size_t start = pos_;

    if (isHexNumberStart(current(), peek())) {
        advance(); // 0
        advance(); // x

        while (!isAtEnd() && isHexDigit(current())) {
            advance();
        }

        if (!isTokenSeparator(current(), peek())) {
            fail(location, "expected token separator after number");
        }
    } 
    else {
        fail(location, "incorrect representation of a number, need hexadecimal");
    }

    std::string lexeme = source_.substr(start, pos_ - start);
    const std::uint64_t value = parseInteger(lexeme, location);

    return Token{
        .type = TokenType::Number,
        .lexeme = lexeme,
        .location = location,
        .numberValue = value
    };
}

bool Lexer::isAtEnd() const noexcept {
    return pos_ >= source_.size();
}

char Lexer::current() const noexcept {
    return isAtEnd() ? '\0' : source_[pos_];
}

char Lexer::peek(std::size_t offset) const noexcept {
    const std::size_t index = pos_ + offset;
    return index < source_.size() ? source_[index] : '\0';
}

char Lexer::advance() noexcept {
    if (isAtEnd()) {
        return '\0';
    }

    const char ch = current();
    const char next_ch = peek();

    if (ch == '\r' && next_ch == '\n') {
        pos_ += 2;
        line_++;
        column_ = 1;
        return '\n';
    }

    pos_++;
    if (ch == '\n') {
        line_++;
        column_ = 1;
    } 
    else {
        column_++;
    }

    return ch;
}

void Lexer::skipWhitespace() noexcept {
    while (!isAtEnd()) {
        const char ch = current();
        if (ch == ' ' || ch == '\t') {
            advance();
        } 
        else {
            break;
        }
    }
}

void Lexer::skipComment() noexcept {
    while (!isAtEnd() && !isNewLine(current(), peek())) {
        advance();
    }
}

Token Lexer::makeSimpleToken(TokenType type, std::string lexeme, SourceLocation location) const {
    return Token{
        .type = type,
        .lexeme = std::move(lexeme),
        .location = location,
        .numberValue = std::nullopt
    };
}

[[noreturn]] void Lexer::fail(const SourceLocation& location, const std::string& message) const {
    throw std::runtime_error(formatErrorMessage(location, message));
}

bool Lexer::isTokenSeparator(char ch, char next_ch) {
    return ch == '\0' || ch == ' ' || ch == '\t' || ch == '\n' || (ch == '\r' && next_ch == '\n');
}

bool Lexer::isCommentStart(const char ch) {
    return ch == comment;
}

bool Lexer::isOperationLike(const std::string& upperLexeme) {
    return common::operationFromSring(upperLexeme).has_value();
}

std::optional<std::int64_t> Lexer::parseRegisterIndex(std::string_view upperLexeme) {
    if (upperLexeme.size() < 2 || upperLexeme[0] != 'R') {
        return std::nullopt;
    }

    if (!std::all_of(upperLexeme.begin() + 1, upperLexeme.end(),
                    [](char ch) {
                        return std::isdigit(static_cast<unsigned char>(ch)) != 0;
                    })) {
        return std::nullopt;
    }

    try {
        std::size_t consumed = 0;
        const auto value = std::stoul(std::string(upperLexeme.substr(1)), &consumed, 10);

        if (consumed != upperLexeme.size() - 1) {
            return std::nullopt;
        }

        return value;
    } 
    catch (...) {
        return std::nullopt;
    }
}

std::string Lexer::toUpperCopy(std::string_view text) {
    std::string result(text);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
    return result;
}

std::uint64_t Lexer::parseInteger(const std::string& lexeme, const SourceLocation& location) {
    try {
        std::size_t consumed = 0;
        std::uint64_t value = 0;

        if (lexeme.size() > 2 && isHexNumberStart(lexeme[0], lexeme[1])) {
            value = std::stoull(lexeme, &consumed, 16);
        } 

        if (consumed != lexeme.size()) {
            throw std::invalid_argument("literal was not fully consumed");
        }

        if (value > std::numeric_limits<std::uint64_t>::max()) {
            throw std::invalid_argument("integer literal is too large for uint64");
        }

        return value;
    } 
    catch (const std::exception&) {
        fail(location, "invalid numeric literal '" + lexeme + "'");
    }
}

}
