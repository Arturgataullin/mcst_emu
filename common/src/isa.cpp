#include "isa.h"

namespace common {
    std::optional<Operation> operationFromSring(std::string_view lexeme) {
        if (lexeme == "LI") {
            return Operation::LI;
        }
        if (lexeme == "ADD") {
            return Operation::ADD;
        }
        return std::nullopt;
    }

    std::string_view toString(Operation op) {
        switch (op) {
            case Operation::LI:  return "LI";
            case Operation::ADD: return "ADD";
        }
        return "UNKNOWN";
    }
}