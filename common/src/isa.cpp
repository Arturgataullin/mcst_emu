#include "isa.h"

namespace common {
    std::optional<Operation> operationFromString(std::string_view lexeme) {
        if (lexeme == "LI") {
            return Operation::LI;
        }
        if (lexeme == "LUI") {
            return Operation::LUI;
        }
        if (lexeme == "ADD") {
            return Operation::ADD;
        }
        if (lexeme == "SUB") {
            return Operation::SUB;
        }
        if (lexeme == "AND") {
            return Operation::AND;
        }
        if (lexeme == "OR") {
            return Operation::OR;
        }
        if (lexeme == "XOR") {
            return Operation::XOR;
        }
        if (lexeme == "SLL") {
            return Operation::SLL;
        }
        if (lexeme == "SRL") {
            return Operation::SRL;
        }
        if (lexeme == "SRA") {
            return Operation::SRA;
        }
        if (lexeme == "MUL") {
            return Operation::MUL;
        }
        if (lexeme == "UDIV") {
            return Operation::UDIV;
        }
        if (lexeme == "SDIV") {
            return Operation::SDIV;
        }
        if (lexeme == "LDB") {
            return Operation::LDB;
        }
        if (lexeme == "LDH") {
            return Operation::LDH;
        }
        if (lexeme == "LDW") {
            return Operation::LDW;
        }
        if (lexeme == "STB") {
            return Operation::STB;
        }
        if (lexeme == "STH") {
            return Operation::STH;
        }
        if (lexeme == "STW") {
            return Operation::STW;
        }
        if (lexeme == "SXT") {
            return Operation::SXT;
        }
        if (lexeme == "BSWAP") {
            return Operation::BSWAP;
        }
        if (lexeme == "MOV") {
            return Operation::MOV;
        }
        if (lexeme == "NEG") {
            return Operation::NEG;
        }
        if (lexeme == "NOT") {
            return Operation::NOT;
        }
        if (lexeme == "LFI") {
            return Operation::LFI;
        }

        return std::nullopt;
    }

    std::string_view toString(Operation op) {
        switch (op) {
            case Operation::LI:   return "LI";
            case Operation::LUI:  return "LUI";
            case Operation::ADD:  return "ADD";
            case Operation::SUB:  return "SUB";
            case Operation::AND:  return "AND";
            case Operation::OR:   return "OR";
            case Operation::XOR:  return "XOR";
            case Operation::SLL:  return "SLL";
            case Operation::SRL:  return "SRL";
            case Operation::SRA:  return "SRA";
            case Operation::MUL:  return "MUL";
            case Operation::UDIV: return "UDIV";
            case Operation::SDIV: return "SDIV";
            case Operation::LDB:  return "LDB";
            case Operation::LDH:  return "LDH";
            case Operation::LDW:  return "LDW";
            case Operation::STB:  return "STB";
            case Operation::STH:  return "STH";
            case Operation::STW:  return "STW";
            case Operation::SXT:  return "SXT";
            case Operation::BSWAP: return "BSWAP";
            case Operation::MOV:  return "MOV";
            case Operation::NEG:  return "NEG";
            case Operation::NOT: return "NOT";
            case Operation::LFI: return "LFI";
        }

        return "UNKNOWN";
    }

    std::string_view toString(Opcode opcode) {
        switch (opcode) {
            case Opcode::LI:   return "LI";
            case Opcode::LUI:  return "LUI";
            case Opcode::ADD:  return "ADD";
            case Opcode::SUB:  return "SUB";
            case Opcode::AND:  return "AND";
            case Opcode::OR:   return "OR";
            case Opcode::XOR:  return "XOR";
            case Opcode::SLL:  return "SLL";
            case Opcode::SRL:  return "SRL";
            case Opcode::SRA:  return "SRA";
            case Opcode::MUL:  return "MUL";
            case Opcode::UDIV: return "UDIV";
            case Opcode::SDIV: return "SDIV";
            case Opcode::LDB:  return "LDB";
            case Opcode::LDH:  return "LDH";
            case Opcode::LDW:  return "LDW";
            case Opcode::STB:  return "STB";
            case Opcode::STH:  return "STH";
            case Opcode::STW:  return "STW";
            case Opcode::SXT:  return "SXT";
            case Opcode::BSWAP: return "BSWAP";
        }

        return "UNKNOWN";
    }

    Opcode opcodeForOperation(Operation op) {
        switch (op) {
            case Operation::LI:   return Opcode::LI;
            case Operation::LUI:  return Opcode::LUI;
            case Operation::ADD:  return Opcode::ADD;
            case Operation::SUB:  return Opcode::SUB;
            case Operation::AND:  return Opcode::AND;
            case Operation::OR:   return Opcode::OR;
            case Operation::XOR:  return Opcode::XOR;
            case Operation::SLL:  return Opcode::SLL;
            case Operation::SRL:  return Opcode::SRL;
            case Operation::SRA:  return Opcode::SRA;
            case Operation::MUL:  return Opcode::MUL;
            case Operation::UDIV: return Opcode::UDIV;
            case Operation::SDIV: return Opcode::SDIV;
            case Operation::LDB:  return Opcode::LDB;
            case Operation::LDH:  return Opcode::LDH;
            case Operation::LDW:  return Opcode::LDW;
            case Operation::STB:  return Opcode::STB;
            case Operation::STH:  return Opcode::STH;
            case Operation::STW:  return Opcode::STW;
            case Operation::SXT:  return Opcode::SXT;
            case Operation::BSWAP: return Opcode::BSWAP;
            default:
                throw std::logic_error("unknown operation");
        }
    }
}
