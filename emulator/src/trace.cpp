#include "trace.h"

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


    std::uint64_t parseTick(std::string_view text) {
        if (text.empty()) {
            throw std::invalid_argument("empty tick in --trace-ticks");
        }

        std::uint64_t value = 0;
        const char* first = text.data();
        const char* last = first + text.size();
        const auto result = std::from_chars(first, last, value);

        if (result.ec != std::errc{} || result.ptr != last) {
            throw std::invalid_argument(
                "invalid tick in --trace-ticks: " + std::string(text)
            );
        }
        return value;
    }

    std::vector<TickRange> normalizeRanges(std::vector<TickRange> ranges) {
        // после сортировки пересечения можно объединить за один проход
        std::sort(ranges.begin(), ranges.end(), [](const TickRange& lhs, const TickRange& rhs) {
            if (lhs.begin != rhs.begin) {
                return lhs.begin < rhs.begin;
            }
            return lhs.end < rhs.end;
        });

        std::vector<TickRange> merged;
        for (const TickRange& range : ranges) {
            if (range.begin > range.end) {
                throw std::invalid_argument("trace tick range is reversed");
            }
            if (merged.empty()) {
                merged.push_back(range);
                continue;
            }

            TickRange& previous = merged.back();
            // соседние диапазоны эквивалентны одному непрерывному диапазону
            const bool adjacent =
                previous.end != std::numeric_limits<std::uint64_t>::max() && range.begin == previous.end + 1;

            if (range.begin <= previous.end || adjacent) {
                previous.end = std::max(previous.end, range.end);
            } 
            else {
                merged.push_back(range);
            }
        }
        return merged;
    }


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

std::vector<TickRange> parseTickRanges(std::string_view text) {
    if (text.empty()) {
        throw std::invalid_argument("--trace-ticks requires a non-empty value");
    }

    std::vector<TickRange> ranges;
    std::size_t position = 0;

    while (position <= text.size()) {
        const std::size_t comma = text.find(',', position);
        const std::size_t tokenEnd =
            comma == std::string_view::npos ? text.size() : comma;
        const std::string_view token = text.substr(position, tokenEnd - position);

        if (token.empty()) {
            throw std::invalid_argument("empty range in --trace-ticks");
        }

        const std::size_t dash = token.find('-');
        if (dash == std::string_view::npos) {
            const std::uint64_t tick = parseTick(token);
            ranges.push_back({tick, tick});
        } 
        else {
            if (token.find('-', dash + 1) != std::string_view::npos) {
                throw std::invalid_argument(
                    "invalid range in --trace-ticks: " + std::string(token)
                );
            }

            const std::uint64_t begin = parseTick(token.substr(0, dash));
            const std::uint64_t end = parseTick(token.substr(dash + 1));
            if (begin > end) {
                throw std::invalid_argument(
                    "reversed range in --trace-ticks: " + std::string(token)
                );
            }
            ranges.push_back({begin, end});
        }

        if (comma == std::string_view::npos) {
            break;
        }
        position = comma + 1;
    }

    return normalizeRanges(std::move(ranges));
}

bool containsTick(const std::vector<TickRange>& ranges, std::uint64_t tick) noexcept {
    // пустой список диапазонов означает трассировку всех тактов
    if (ranges.empty()) {
        return true;
    }

    for (const TickRange& range : ranges) {
        if (tick < range.begin) {
            return false;
        }
        if (tick <= range.end) {
            return true;
        }
    }
    return false;
}

void Emulator::enableDisasmTrace(std::ostream& output, std::vector<TickRange> ranges) {
    traceOutput_ = &output;
    // публичный метод может получить диапазоны не только из CLI
    traceRanges_ = normalizeRanges(std::move(ranges));
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
