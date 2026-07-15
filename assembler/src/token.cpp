#include "token.h"

namespace assembler {
std::string_view toString(TokenType type) {
    switch (type) {
        case TokenType::Operation: return "Operation";
        case TokenType::Register: return "Register";
        case TokenType::StatusRegister: return "StatusRegister";
        case TokenType::Number: return "Number";
        case TokenType::NewLine: return "NewLine";
        case TokenType::EndOfFile: return "EndOfFile";
    }

    return "UnknownTokenType";
}
}
