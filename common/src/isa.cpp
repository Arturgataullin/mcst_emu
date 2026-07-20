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
        if (lexeme == "SCRW") {
            return Operation::SCRW;
        }
        if (lexeme == "SCRR") {
            return Operation::SCRR;
        }
        if (lexeme == "ASPI") {
            return Operation::ASPI;
        }
        if (lexeme == "ASPR") {
            return Operation::ASPR;
        }
        if (lexeme == "RJMP") {
            return Operation::RJMP;
        }
        if (lexeme == "BRZ") {
            return Operation::BRZ;
        }
        if (lexeme == "BRNZ") {
            return Operation::BRNZ;
        }
        if (lexeme == "AJMP") {
            return Operation::AJMP;
        }
        if (lexeme == "CALL") {
            return Operation::CALL;
        }
        if (lexeme == "EQ") {
            return Operation::EQ;
        }
        if (lexeme == "NE") {
            return Operation::NE;
        }
        if (lexeme == "LT") {
            return Operation::LT;
        }
        if (lexeme == "GE") {
            return Operation::GE;
        }
        if (lexeme == "SLT") {
            return Operation::SLT;
        }
        if (lexeme == "SGE") {
            return Operation::SGE;
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
        if (lexeme == "RET") {
            return Operation::RET;
        }

        return std::nullopt;
    }

    std::optional<StatusRegister> statusRegisterFromString(std::string_view lexeme) {
        if (lexeme == "IP") {
            return StatusRegister::Ip;
        }
        if (lexeme == "SP_TOP") {
            return StatusRegister::SpTop;
        }
        if (lexeme == "SP_SIZE") {
            return StatusRegister::SpSize;
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
            case Operation::SCRW: return "SCRW";
            case Operation::SCRR: return "SCRR";
            case Operation::ASPI: return "ASPI";
            case Operation::ASPR: return "ASPR";
            case Operation::RJMP: return "RJMP";
            case Operation::BRZ: return "BRZ";
            case Operation::BRNZ: return "BRNZ";
            case Operation::AJMP: return "AJMP";
            case Operation::CALL: return "CALL";
            case Operation::EQ: return "EQ";
            case Operation::NE: return "NE";
            case Operation::LT: return "LT";
            case Operation::GE: return "GE";
            case Operation::SLT: return "SLT";
            case Operation::SGE: return "SGE";
            case Operation::MOV:  return "MOV";
            case Operation::NEG:  return "NEG";
            case Operation::NOT: return "NOT";
            case Operation::LFI: return "LFI";
            case Operation::RET: return "RET";
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
            case Opcode::SCRW: return "SCRW";
            case Opcode::SCRR: return "SCRR";
            case Opcode::ASPI: return "ASPI";
            case Opcode::ASPR: return "ASPR";
            case Opcode::RJMP: return "RJMP";
            case Opcode::BRZ: return "BRZ";
            case Opcode::BRNZ: return "BRNZ";
            case Opcode::AJMP: return "AJMP";
            case Opcode::CALL: return "CALL";
            case Opcode::EQ: return "EQ";
            case Opcode::NE: return "NE";
            case Opcode::LT: return "LT";
            case Opcode::GE: return "GE";
            case Opcode::SLT: return "SLT";
            case Opcode::SGE: return "SGE";
        }

        return "UNKNOWN";
    }

    std::string_view toString(StatusRegister reg) {
        switch (reg) {
            case StatusRegister::Ip: return "IP";
            case StatusRegister::SpTop: return "SP_TOP";
            case StatusRegister::SpSize: return "SP_SIZE";
            case StatusRegister::Count: break;
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
            case Operation::SCRW: return Opcode::SCRW;
            case Operation::SCRR: return Opcode::SCRR;
            case Operation::ASPI: return Opcode::ASPI;
            case Operation::ASPR: return Opcode::ASPR;
            case Operation::RJMP: return Opcode::RJMP;
            case Operation::BRZ: return Opcode::BRZ;
            case Operation::BRNZ: return Opcode::BRNZ;
            case Operation::AJMP: return Opcode::AJMP;
            case Operation::CALL: return Opcode::CALL;
            case Operation::EQ: return Opcode::EQ;
            case Operation::NE: return Opcode::NE;
            case Operation::LT: return Opcode::LT;
            case Operation::GE: return Opcode::GE;
            case Operation::SLT: return Opcode::SLT;
            case Operation::SGE: return Opcode::SGE;
            default:
                throw std::logic_error("unknown operation");
        }
    }
}
