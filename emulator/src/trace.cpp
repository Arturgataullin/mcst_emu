#include "ranges.h"

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

    void writeHexWord(std::ostream& out, std::uint32_t value) {
        out << "0x"
            << std::hex
            << std::nouppercase
            << std::right
            << std::setw(8)
            << std::setfill('0')
            << value
            << std::setfill(' ');
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

    void writeStatusRegisterOperand(
        std::ostream& out,
        std::uint8_t index,
        std::uint32_t value
    ) {
        out << ' '
            << common::toString(static_cast<common::StatusRegister>(index))
            << " (";
        writeHexValue(out, value);
        out << ')';
    }

}

void Emulator::enableDisasmTrace(std::ostream& output) {
    traceOutput_ = &output;
}

void Emulator::setTraceTicks(std::vector<TickRange> ranges) {
    tickRangeFilter_.setRanges(std::move(ranges));
}

void Emulator::enableRamWriteTrace(std::ostream& output, std::vector<AddressRange> ranges) {
    ramWriteTraceOutput_ = &output;
    ramWriteAddressFilter_.setRanges(std::move(ranges));
    // одна невыровненная запись может задеть две соседние ячейки
    pendingRamWriteTrace_.reserve(2);
    // callback подключается только при включенном ram-wr, чтобы не замедлять обычные записи
    memory_.setCellWriteHandler([this](
        std::uint32_t cellAddress,
        std::uint32_t oldData,
        std::uint32_t newData
    ) {
        // memory сообщает только факт изменения ячейки, а фильтры и формат остаются в Emulator
        collectRamWriteTrace(cellAddress, oldData, newData);
    });
}

void Emulator::collectRamWriteTrace(
    std::uint32_t cellAddress,
    std::uint32_t oldData,
    std::uint32_t newData
) {
    if (ramWriteTraceOutput_ == nullptr) {
        return;
    }

    const std::uint32_t cellEndAddress =
        cellAddress + static_cast<std::uint32_t>(Memory::cellSize - 1);

    // фильтр адресов смотрит на пересечение с байтами ячейки, а не только на адрес начала ячейки
    if (!tickRangeFilter_.contains(tick_) ||
        !ramWriteAddressFilter_.intersects(cellAddress, cellEndAddress)) {
        return;
    }

    pendingRamWriteTrace_.push_back({
        tick_,
        cellAddress,
        oldData,
        newData
    });
}

void Emulator::writeRamWriteTrace(const RamWriteTraceEvent& event) const {
    std::ostream& out = *ramWriteTraceOutput_;
    StreamStateGuard guard(out);

    out << std::dec
        << event.tick
        << " RAM [";
    writeHexWord(out, event.cellAddress);
    out << "] wr ";
    writeHexWord(out, event.oldData);
    out << " -> ";
    writeHexWord(out, event.newData);
    out << '\n';
}

void Emulator::flushRamWriteTrace() {
    if (ramWriteTraceOutput_ == nullptr) {
        pendingRamWriteTrace_.clear();
        return;
    }

    // порядок событий совпадает с порядком уведомлений от Memory
    for (const RamWriteTraceEvent& event : pendingRamWriteTrace_) {
        writeRamWriteTrace(event);
    }
    pendingRamWriteTrace_.clear();
}

void Emulator::writeDisasmTrace(
    std::uint64_t tick,
    std::uint32_t address,
    std::uint32_t encoding,
    const DecodedInstruction& instruction,
    const TraceSnapshot& before
) const {
    // регистры назначения читаются после execute(), а регистры-источники - из снимка до execute()
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
        case common::Opcode::EQ:
        case common::Opcode::NE:
        case common::Opcode::LT:
        case common::Opcode::GE:
        case common::Opcode::SLT:
        case common::Opcode::SGE:
            writeRegisterOperand(out, instruction.a, readRegister(instruction.a));
            writeRegisterOperand(out, instruction.b, before.b);
            writeRegisterOperand(out, instruction.c, before.c);
            break;

        // для формата opcode a b i поле c печатается как imm8, а не как регистр
        case common::Opcode::LDB:
        case common::Opcode::LDH:
        case common::Opcode::LDW:
            writeRegisterOperand(out, instruction.a, readRegister(instruction.a));
            writeRegisterOperand(out, instruction.b, before.b);
            out << " imm (0x" << std::hex << static_cast<unsigned>(instruction.c) << ')';
            break;

        case common::Opcode::STB:
        case common::Opcode::STH:
        case common::Opcode::STW:
            writeRegisterOperand(out, instruction.a, before.a);
            writeRegisterOperand(out, instruction.b, before.b);
            out << " imm (0x" << std::hex << static_cast<unsigned>(instruction.c) << ')';
            break;

        case common::Opcode::SXT:
        case common::Opcode::BSWAP:
            writeRegisterOperand(out, instruction.a, readRegister(instruction.a));
            writeRegisterOperand(out, instruction.b, before.b);
            out << " imm (0x" << std::hex << static_cast<unsigned>(instruction.c) << ')';
            break;

        case common::Opcode::SCRW:
            out << ' '
                << common::toString(static_cast<common::StatusRegister>(instruction.a));
            writeRegisterOperand(out, instruction.b, before.b);
            break;

        case common::Opcode::SCRR:
            writeRegisterOperand(out, instruction.a, readRegister(instruction.a));
            // SCRR не изменяет SCR, поэтому значение SCR можно читать без предварительного снимка
            writeStatusRegisterOperand(
                out,
                instruction.b,
                readStatusRegister(instruction.b)
            );
            break;

        case common::Opcode::ASPI:
            writeRegisterOperand(out, instruction.a, readRegister(instruction.a));
            out << " imm (0x" << std::hex << instruction.imm << ')';
            break;

        case common::Opcode::ASPR:
            writeRegisterOperand(out, instruction.a, readRegister(instruction.a));
            writeRegisterOperand(out, instruction.b, before.b);
            break;

        case common::Opcode::RJMP:
            out << " imm (0x" << std::hex << instruction.imm << ')';
            break;

        case common::Opcode::BRZ:
        case common::Opcode::BRNZ:
            writeRegisterOperand(out, instruction.a, before.a);
            out << " imm (0x" << std::hex << instruction.imm << ')';
            break;

        case common::Opcode::AJMP:
        case common::Opcode::CALL:
            writeRegisterOperand(out, instruction.a, before.a);
            break;
    }

    out << '\n';
}

}
