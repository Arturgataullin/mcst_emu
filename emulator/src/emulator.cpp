#include "emulator.h"

#include <fstream>
#include <iomanip>
#include <iterator>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace emulator {

Emulator::Emulator(std::size_t memorySize)
    : memory_(memorySize) {}

std::vector<std::uint8_t> Emulator::readBinaryFile(const std::string& inputPath) {
    std::ifstream in(inputPath, std::ios::binary);
    if (!in) {
        throw std::runtime_error("failed to open input file: " + inputPath);
    }

    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void Emulator::loadProgramFromFile(const std::string& inputPath) {
    loadProgram(readBinaryFile(inputPath));
}

void Emulator::loadProgram(const std::vector<std::uint8_t>& program, std::uint32_t baseAddress) {
    if (program.size() % common::instructionSizeBytes != 0) {
        throw std::runtime_error("program size must be a multiple of 4 bytes");
    }

    constexpr std::uint64_t maxAddressableSize =
        static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()) + 1u;
    if (program.size() > maxAddressableSize) {
        throw std::runtime_error("program does not fit 32-bit address space");
    }

    const std::uint64_t base = baseAddress;
    const std::uint64_t programSize = static_cast<std::uint64_t>(program.size());
    if (base > maxAddressableSize - programSize) {
        throw std::runtime_error("program does not fit 32-bit address space");
    }

    registers_.fill(0);
    assemblerTempRegister_ = 0;
    memory_.clear();

    memory_.load(program, baseAddress);

    programBase_ = baseAddress;
    // конец диапазона может быть 0x1'0000'0000, поэтому хранится шире гостевого адреса
    programEnd_ = base + programSize;
    pc_ = baseAddress;
    tick_ = 0;

#if MCST_TRACING
    tickRangeFilter_.reset();
#endif
}

const std::array<std::uint32_t, common::registerCount>& Emulator::registers() const noexcept {
    return registers_;
}

std::uint64_t Emulator::pc() const noexcept {
    return pc_;
}

bool Emulator::isFinished() const noexcept {
    return pc_ >= programEnd_;
}

std::uint32_t Emulator::fetchInstructionWord() const {
    if (pc_ + common::instructionSizeBytes > programEnd_) {
        throw std::runtime_error("instruction fetch goes past loaded program");
    }

    return memory_.read32(static_cast<std::uint32_t>(pc_));
}

DecodedInstruction Emulator::decode(std::uint32_t word) const {
    DecodedInstruction inst;
    inst.opcode = static_cast<common::Opcode>(word & 0xFF);
    inst.a = static_cast<std::uint8_t>((word >> 8) & 0xFF);
    inst.b = static_cast<std::uint8_t>((word >> 16) & 0xFF);
    inst.c = static_cast<std::uint8_t>((word >> 24) & 0xFF);
    inst.imm = static_cast<std::uint16_t>(inst.b | (static_cast<std::uint16_t>(inst.c) << 8));
    return inst;
}

void Emulator::validateRegisterIndex(std::uint8_t reg, const char* fieldName) const {
    if (reg >= common::registerCount && reg != common::assemblerTempRegister) {
        throw std::runtime_error(std::string("invalid register index in ") + fieldName);
    }
}

std::uint32_t Emulator::readRegister(std::uint8_t reg) const {
    if (reg == common::assemblerTempRegister) {
        return assemblerTempRegister_;
    }
    return registers_[reg];
}

void Emulator::writeRegister(std::uint8_t reg, std::uint32_t value) {
    if (reg == common::assemblerTempRegister) {
        assemblerTempRegister_ = value;
        return;
    }
    registers_[reg] = value;
}

namespace {

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

// реализация сдвига без преобразования
std::uint32_t sra_without_cast(std::uint32_t val, std::uint32_t shift) {
    shift &= 0x1F;

    if (shift == 0) {
        return val;
    }
    std::uint32_t shifted = val >> shift;
    const bool sign = val & 0x80000000u; 
    if (!sign) { 
        return shifted;
    }
    const std::uint32_t mask = ~std::uint32_t{0} << (common::registerSize - shift);
    return shifted | mask;
} 

std::uint32_t sign_extend_byte(std::uint32_t value) {
    // sxt для байта смотрит только на младшие 8 бит исходного регистра
    value &= 0x000000FFu;
    if ((value & 0x00000080u) == 0) {
        return value;
    }
    return value | 0xFFFFFF00u;
}

std::uint32_t sign_extend_halfword(std::uint32_t value) {
    // sxt для полуслова смотрит на бит 15 и заполняет старшие 16 бит этим знаком
    value &= 0x0000FFFFu;
    if ((value & 0x00008000u) == 0) {
        return value;
    }
    return value | 0xFFFF0000u;
}

std::uint32_t sign_extend(std::uint32_t value, std::uint8_t mode) {
    // по заданию остальные режимы sxt игнорируются и оставляют значение без изменений
    switch (mode) {
        case 0:
            return sign_extend_byte(value);
        case 1:
            return sign_extend_halfword(value);
        default:
            return value;
    }
}

std::uint32_t byte_swap_halfword(std::uint32_t value) {
    // режим 1 меняет местами только два младших байта, старшие 16 бит сохраняются
    return (value & 0xFFFF0000u) |
           ((value & 0x000000FFu) << 8) |
           ((value & 0x0000FF00u) >> 8);
}

std::uint32_t byte_swap_word(std::uint32_t value) {
    return ((value & 0x000000FFu) << 24) |
           ((value & 0x0000FF00u) << 8) |
           ((value & 0x00FF0000u) >> 8) |
           ((value & 0xFF000000u) >> 24);
}

std::uint32_t byte_swap(std::uint32_t value, std::uint8_t mode) {
    // по заданию остальные режимы bswap игнорируются и оставляют значение без изменений
    switch (mode) {
        case 1:
            return byte_swap_halfword(value);
        case 2:
            return byte_swap_word(value);
        default:
            return value;
    }
}

}

void Emulator::enableUninitializedRamWarnings(std::ostream& output) {
    uninitRamWarningOutput_ = &output;
    memory_.setUninitReadHandler([this](
        std::uint32_t address,
        std::size_t byteCount,
        std::uint8_t uninitMask
    ) {
        // memory сообщает только адреса и маску, а tick, pc и формат предупреждения остаются в Emulator
        writeUninitRamWarning(address, byteCount, uninitMask);
    });
}

void Emulator::writeUninitRamWarning(
    std::uint32_t address,
    std::size_t byteCount,
    std::uint8_t uninitMask
) const {
    if (uninitRamWarningOutput_ == nullptr) {
        return;
    }

    std::ostream& out = *uninitRamWarningOutput_;
    StreamStateGuard guard(out);

    out << std::dec
        << tick_
        << " WARN uninit-ram pc=";
    writeHexWord(out, static_cast<std::uint32_t>(pc_));
    out << " read "
        << byteCount
        << " byte(s) at ";
    writeHexWord(out, address);
    out << " uninit:";

    bool first = true;
    for (std::size_t i = 0; i < byteCount; ++i) {
        if ((uninitMask & (1u << i)) == 0) {
            continue;
        }

        out << (first ? ' ' : ',');
        writeHexWord(out, address + static_cast<std::uint32_t>(i));
        first = false;
    }

    out << '\n';
}

void Emulator::execute(const DecodedInstruction& instruction) {
    auto advancePc = [this]() {
        pc_ += common::instructionSizeBytes;
    };

    switch (instruction.opcode) {
        case common::Opcode::LI:
        case common::Opcode::LUI: { 
            validateRegisterIndex(instruction.a, "destination");
            switch (instruction.opcode) {
                case common::Opcode::LI:
                    writeRegister(instruction.a, instruction.imm);
                    break;
                case common::Opcode::LUI: {
                    const auto lower = readRegister(instruction.a) & 0xFFFFu;
                    writeRegister(
                        instruction.a,
                        lower | (static_cast<std::uint32_t>(instruction.imm) << 16)
                    );
                    break;
                }
                default:
                    throw std::logic_error("unreachable opcode in RV block");
            }
            advancePc();
            return;
        }

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
        case common::Opcode::SDIV: {
            validateRegisterIndex(instruction.a, "destination");
            validateRegisterIndex(instruction.b, "left source");
            validateRegisterIndex(instruction.c, "right source");

            const std::uint32_t lhs = readRegister(instruction.b);
            const std::uint32_t rhs = readRegister(instruction.c);

            std::uint32_t result = 0;

            switch (instruction.opcode) {
                case common::Opcode::ADD:
                    result = lhs + rhs;
                    break;
                case common::Opcode::SUB:
                    result = lhs - rhs;
                    break;
                case common::Opcode::AND:
                    result = lhs & rhs;
                    break;
                case common::Opcode::OR:
                    result = lhs | rhs;
                    break;
                case common::Opcode::XOR:
                    result = lhs ^ rhs;
                    break;
                case common::Opcode::SLL:
                    result = lhs << (rhs & 0x1F); // for 32 bit register 
                    break;
                case common::Opcode::SRL:
                    result = lhs >> (rhs & 0x1F);
                    break;
                case common::Opcode::SRA:
                    result = sra_without_cast(lhs, rhs);
                    break;
                case common::Opcode::MUL:
                    result = lhs * rhs;
                    break;
                case common::Opcode::UDIV:
                    if (rhs == 0) {
                        throw std::runtime_error("UDIV: division by zero");
                    }
                    result = lhs / rhs;
                    break;
                case common::Opcode::SDIV: {
                    const auto signedLhs = static_cast<std::int32_t>(lhs);
                    const auto signedRhs = static_cast<std::int32_t>(rhs);

                    if (signedRhs == 0) {
                        throw std::runtime_error("SDIV: division by zero");
                    }
                    result = static_cast<std::uint32_t>(
                        static_cast<std::int64_t>(signedLhs) / //cast to int64 for -2147483648 / -1
                        static_cast<std::int64_t>(signedRhs)
                    );
                    break;
                }
                default:
                    throw std::logic_error("unreachable opcode in RRR block");
            }

            writeRegister(instruction.a, result);
            advancePc();
            return;
        }

        case common::Opcode::LDB:
        case common::Opcode::LDH:
        case common::Opcode::LDW: {
            validateRegisterIndex(instruction.a, "destination");
            validateRegisterIndex(instruction.b, "base address");

            // в формате opcode a b i поле c хранит imm8, поэтому адрес считается как r[b] + c
            const std::uint32_t address = readRegister(instruction.b) + instruction.c;
            std::uint32_t result = 0;

            switch (instruction.opcode) {
                case common::Opcode::LDB:
                    result = memory_.read8(address);
                    break;
                case common::Opcode::LDH:
                    result = memory_.read16(address);
                    break;
                case common::Opcode::LDW:
                    result = memory_.read32(address);
                    break;
                default:
                    throw std::logic_error("unreachable opcode in load block");
            }

            writeRegister(instruction.a, result);
            advancePc();
            return;
        }

        case common::Opcode::STB:
        case common::Opcode::STH:
        case common::Opcode::STW: {
            validateRegisterIndex(instruction.a, "source");
            validateRegisterIndex(instruction.b, "base address");

            // store берёт данные из r[a], а r[b] используется только как база адреса
            const std::uint32_t value = readRegister(instruction.a);
            const std::uint32_t address = readRegister(instruction.b) + instruction.c;

            switch (instruction.opcode) {
                case common::Opcode::STB:
                    memory_.write8(address, static_cast<std::uint8_t>(value & 0xFFu));
                    break;
                case common::Opcode::STH:
                    memory_.write16(address, static_cast<std::uint16_t>(value & 0xFFFFu));
                    break;
                case common::Opcode::STW:
                    memory_.write32(address, value);
                    break;
                default:
                    throw std::logic_error("unreachable opcode in store block");
            }

            advancePc();
            return;
        }

        case common::Opcode::SXT:
        case common::Opcode::BSWAP: {
            validateRegisterIndex(instruction.a, "destination");
            validateRegisterIndex(instruction.b, "source");

            // sxt и bswap не обращаются к памяти: поле c здесь задаёт режим операции
            const std::uint32_t source = readRegister(instruction.b);
            std::uint32_t result = 0;

            switch (instruction.opcode) {
                case common::Opcode::SXT:
                    result = sign_extend(source, instruction.c);
                    break;
                case common::Opcode::BSWAP:
                    result = byte_swap(source, instruction.c);
                    break;
                default:
                    throw std::logic_error("unreachable opcode in conversion block");
            }

            writeRegister(instruction.a, result);
            advancePc();
            return;
        }
        default:
            throw std::runtime_error("invalid opcode");
    }
}

void Emulator::step() {
    if (isFinished()) {
        return;
    }

    const std::uint32_t instructionAddress = static_cast<std::uint32_t>(pc_);
    (void) instructionAddress;
    const std::uint32_t word = fetchInstructionWord();
    const DecodedInstruction instruction = decode(word);

#if MCST_TRACING
    // номер такта равен числу команд, исполненных перед текущей командой
    const std::uint64_t currentTick = tick_;
    const bool emitRamWriteTrace = ramWriteTraceOutput_ != nullptr;
    if (emitRamWriteTrace) {
        // на каждом шаге буфер должен содержать только записи текущей инструкции
        pendingRamWriteTrace_.clear();
    }
    const bool emitDisasmTrace =
        traceOutput_ != nullptr &&
        tickRangeFilter_.contains(currentTick);

    StateSnapshot before{};
    if (emitDisasmTrace) {
        // источники должны выводиться со значениями до исполнения команды
        before.registers = registers_;
        before.tempRegister = assemblerTempRegister_;
    }
#endif

    execute(instruction);

    // счётчик увеличивается только после успешного выполнения инструкции
    ++tick_;

#if MCST_TRACING
    if (emitDisasmTrace) {
        writeDisasmTrace(
            currentTick,
            instructionAddress - programBase_,
            word,
            instruction,
            before
        );
    }
    if (emitRamWriteTrace) {
        // RAM trace печатается после execute(), когда известны все изменения ячеек
        flushRamWriteTrace();
    }
#endif
}

void Emulator::run() {
    while (!isFinished()) {
        step();
    }
}

std::string Emulator::dumpRegisters() const {
    std::ostringstream out;

    for (std::size_t i = 0; i < registers_.size(); ++i) {
        if (i != 0) {
            out << ", ";
        }

        out << "R" << std::dec << i << "=0x" << std::hex << std::nouppercase << registers_[i];
    }

    return out.str();
}

}
