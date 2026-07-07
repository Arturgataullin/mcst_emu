#include "tick_ranges.h"

#include "emulator.h"

#include <algorithm>
#include <charconv>
#include <iomanip>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace emulator {
namespace {
    
    // raii класс для сохранения и восстановления состояни потока
    class StreamStateGuard {
    public:
        explicit StreamStateGuard(std::ostream& out)
        : out_(out),
            flags_(out.flags()),
            fill_(out.fill()),
            width_(out.width()) {
                out_.width(0);
            }
        
        ~StreamStateGuard() {
            out_.flags(flags_);
            out_.fill(fill_);
            out_.width(width_);
        }

        StreamStateGuard(const StreamStateGuard&) = delete;
        StreamStateGuard& operator=(const StreamStateGuard&) = delete;

    private:
        std::ostream& out_;
        std::ios_base::fmtflags flags_;
        char fill_;
        std::streamsize width_;
    };

    void writeHexValue(std::ostream& out, std::uint32_t value) {
        out << "0x"
            << std::hex
            << std::nouppercase
            << value;
    }   

    void writeRegisterOperand(
        std::ostream& out, 
        std::uint8_t reg,
        std::uint32_t value
    ) {
        out << " R" 
            << std::dec 
            << static_cast<unsigned>(reg)
            << " (";
            writeHexValue(out, value);
            out << ')';
    }

}

void Emulator::setTraceOutput(std::ostream& output) {
    traceOutput_ = &output;
}

void Emulator::setTraceRanges(std::vector<TickRange> ranges) {
    tickRangeFilter_.setRanges(std::move(ranges));
}

void Emulator::writeDisasmTrace(
    std::uint64_t tick,
    std::uint32_t address,
    std::uint32_t encoding,
    const DecodedInstruction& instruction,
    const StateSnapshot& before
) const {
    // приёмник читается после execute(), а источники - из снимка до execute()
    const auto readBefore = [&before](std::uint8_t reg) {
        if (reg == common::assemblerTempRegister) {
            return before.tempRegister;
        }
        return before.registers[reg];
    };

    // использую основной поток вывода, но сохраняю его состояние и флаги
    std::ostream& out = *traceOutput_;
    StreamStateGuard guard(out);

    out << "---- " 
        << std::dec 
        << tick
        << " 0x" 
        << std::hex 
        << std::nouppercase 
        << address
        << " ----\n";
    // encoding печатается как одно 32-битное слово из восьми hex-цифр
    out << "0x" 
        << std::hex 
        << std::nouppercase
        << std::setw(8) 
        << std::setfill('0') 
        << encoding
        << std::setfill(' ') 
        << ' ' 
        << common::toString(instruction.opcode);

    switch (instruction.opcode) {
        case common::Opcode::LI:
        case common::Opcode::LUI:
            writeRegisterOperand(out, instruction.a, readRegister(instruction.a));
            out << " imm (0x" << std::hex << instruction.imm << ')';
            break;

        case common::Opcode::ADD:
        case common::Opcode::SUB:
        case common::Opcode::AND:
        case common::Opcode::OR:
        case common::Opcode::XOR:
        case common::Opcode::SLL:
        case common::Opcode::SRL:
        case common::Opcode::SRA:
        case common::Opcode::MUL:
        case common::Opcode::UDIV:
        case common::Opcode::SDIV:
            writeRegisterOperand(out, instruction.a, readRegister(instruction.a));
            writeRegisterOperand(out, instruction.b, readBefore(instruction.b));
            writeRegisterOperand(out, instruction.c, readBefore(instruction.c));
            break;
    }

    out << '\n';
}

}
