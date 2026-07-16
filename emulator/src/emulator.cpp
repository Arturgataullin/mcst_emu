#include "emulator.h"

#include <fstream>
#include <iomanip>
#include <iterator>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace emulator {

namespace {

[[noreturn]] void throwInvalidRegisterOndexIn(const char* fieldName) {
    throw std::runtime_error(std::string("invalid register index in ") + fieldName);
}

[[noreturn]] void throwInstructionFetchGoesPastLoadedProgram() {
    throw std::runtime_error("instruction fetch goes past loaded program");
}

[[noreturn]] void throwUDIVDivByZero() {
    throw std::runtime_error("UDIV: division by zero");
}

[[noreturn]] void throwSDIVDivByZero() {
    throw std::runtime_error("SDIV: division by zero");
}

[[noreturn]] void throwInvalidOpcode() {
    throw std::runtime_error("invalid opcode");
}

[[noreturn]] void throwInvalidStatusRegisterIndex() {
    throw std::runtime_error("invalid status register index");
}

[[noreturn]] void throwStackOverflow() {
    throw std::runtime_error("stack overflow");
}

[[noreturn]] void throwStackUnderflow() {
    throw std::runtime_error("stack underflow");
}

}

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
    stackBottom_ = 0;
    statusRegisters_.fill(0);
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
    if (pc_ + common::instructionSizeBytes > programEnd_) [[unlikely]] {
        throwInstructionFetchGoesPastLoadedProgram();
    }

    return memory_.read32(static_cast<std::uint32_t>(pc_));
}

inline DecodedInstruction Emulator::decode(std::uint32_t word) const {
    DecodedInstruction inst;
    inst.opcode = static_cast<common::Opcode>(word & 0xFF);
    inst.a = static_cast<std::uint8_t>((word >> 8) & 0xFF);
    inst.b = static_cast<std::uint8_t>((word >> 16) & 0xFF);
    inst.c = static_cast<std::uint8_t>((word >> 24) & 0xFF);
    inst.imm = static_cast<std::uint16_t>(inst.b | (static_cast<std::uint16_t>(inst.c) << 8));
    return inst;
}

inline void Emulator::validateRegisterIndex(std::uint8_t reg, const char* fieldName) const {
    if (reg >= common::registerCount && reg != common::assemblerTempRegister) [[unlikely]] {
        throwInvalidRegisterOndexIn(fieldName);
    }
}

inline std::uint32_t Emulator::readRegister(std::uint8_t reg) const {
    if (reg == common::assemblerTempRegister) [[unlikely]] {
        return assemblerTempRegister_;
    }
    return registers_[reg];
}

inline void Emulator::writeRegister(std::uint8_t reg, std::uint32_t value) {
    if (reg == common::assemblerTempRegister) [[unlikely]] {
        assemblerTempRegister_ = value;
        return;
    }
    registers_[reg] = value;
}

inline std::uint32_t Emulator::readStatusRegister(common::StatusRegister reg) const {
    const std::size_t index = common::statusRegisterIndex(reg);

    // нулевой индекс пока зарезервирован под IP, который будет добавлен вместе с переходами
    if (index == 0 || index >= statusRegisters_.size()) [[unlikely]] {
        throwInvalidStatusRegisterIndex();
    }

    return statusRegisters_[index];
}

inline void Emulator::writeStatusRegister(common::StatusRegister reg, std::uint32_t value) {
    const std::size_t index = common::statusRegisterIndex(reg);

    if (index == 0 || index >= statusRegisters_.size()) [[unlikely]] {
        throwInvalidStatusRegisterIndex();
    }

    statusRegisters_[index] = value;
}

void Emulator::advanceStackPointer(std::uint8_t destination, std::int64_t delta) {
    validateRegisterIndex(destination, "destination");

    // delta уже расширен до int64_t, поэтому -INT32_MIN не вызывает знаковое переполнение
    const std::uint32_t spTop = readStatusRegister(common::StatusRegister::SpTop);
    const std::uint32_t spSize = readStatusRegister(common::StatusRegister::SpSize);
    if (delta < 0) {
        const std::int64_t allocation = -delta;
        if (allocation > spSize) [[unlikely]] {
            throwStackOverflow();
        }
    }

    // положительное смещение не должно переносить SP_TOP выше исходного начала стека
    const std::int64_t newTopValue = static_cast<std::int64_t>(spTop) + delta;
    if (delta > 0 && newTopValue > stackBottom_) [[unlikely]] {
        throwStackUnderflow();
    }

    // регистры архитектурно 32-битные, итоговое присваивание сохраняет wrap-around семантику
    const std::uint32_t newTop = static_cast<std::uint32_t>(newTopValue);
    const std::uint32_t newSize = static_cast<std::uint32_t>(spSize + delta);

    // обработка освобождённой памяти идёт до изменения SCR, чтобы ошибка диапазона не оставила частичное состояние
    if (delta > 0) {
        const std::size_t releasedSize = static_cast<std::size_t>(delta);
        if (clearStack_) {
            // при очистке стека нули и признаки инициализации сбрасываются за один проход по блокам
            memory_.clearAndMarkUninitialized(spTop, releasedSize);
        }
        else {
            memory_.markUninitialized(spTop, releasedSize);
        }
    }

    writeStatusRegister(common::StatusRegister::SpTop, newTop);
    writeStatusRegister(common::StatusRegister::SpSize, newSize);
    writeRegister(destination, newTop);
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
inline std::uint32_t sraWithoutCast(std::uint32_t val, std::uint32_t shift) {
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

inline std::uint32_t signExtendBits(std::uint32_t value, unsigned bits) {
    const std::uint32_t signBit = std::uint32_t{1} << (bits - 1);
    const std::uint32_t mask = (std::uint32_t{1} << bits) - 1u;

    value &= mask;
    return (value ^ signBit) - signBit;
}

inline std::uint32_t signExtend(std::uint32_t value, std::uint8_t mode) {
    switch (mode) {
        case 0:
            return signExtendBits(value, 8);
        case 1:
            return signExtendBits(value, 16);
        default:
            return value;
    }
}

inline std::uint32_t byteSwapHalfword(std::uint32_t value) {
    // режим 1 меняет местами только два младших байта, старшие 16 бит сохраняются
    return (value & 0xFFFF0000u) |
           ((value & 0x000000FFu) << 8) |
           ((value & 0x0000FF00u) >> 8);
}

inline std::uint32_t byteSwapWord(std::uint32_t value) {
    return ((value & 0x000000FFu) << 24) |
           ((value & 0x0000FF00u) << 8) |
           ((value & 0x00FF0000u) >> 8) |
           ((value & 0xFF000000u) >> 24);
}

inline std::uint32_t byteSwap(std::uint32_t value, std::uint8_t mode) {
    // по заданию остальные режимы bswap игнорируются и оставляют значение без изменений
    switch (mode) {
        case 1:
            return byteSwapHalfword(value);
        case 2:
            return byteSwapWord(value);
        default:
            return value;
    }
}

}

void Emulator::enableClearStack() {
    clearStack_ = true;
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

#if MCST_TRACING

Emulator::TraceSnapshot Emulator::captureTraceSnapshot(const DecodedInstruction& instruction) const {
    TraceSnapshot snapshot{};
    const auto readIfValid = [this](std::uint8_t reg) {
        // если регистр некорректный, execute() позже выдаст ошибку валидации, а trace не будет напечатан
        if (reg >= common::registerCount && reg != common::assemblerTempRegister) {
            return 0u;
        }
        return readRegister(reg);
    };

    switch (instruction.opcode) {
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
            snapshot.b = readIfValid(instruction.b);
            snapshot.c = readIfValid(instruction.c);
            break;

        case common::Opcode::LDB:
        case common::Opcode::LDH:
        case common::Opcode::LDW:
        case common::Opcode::SXT:
        case common::Opcode::BSWAP:
            snapshot.b = readIfValid(instruction.b);
            break;

        case common::Opcode::STB:
        case common::Opcode::STH:
        case common::Opcode::STW:
            snapshot.a = readIfValid(instruction.a);
            snapshot.b = readIfValid(instruction.b);
            break;

        case common::Opcode::SCRW:
        case common::Opcode::ASPR:
            snapshot.b = readIfValid(instruction.b);
            break;

        case common::Opcode::LI:
        case common::Opcode::LUI:
        case common::Opcode::ASPI:
        case common::Opcode::SCRR:
            break;
    }

    return snapshot;
}

#endif

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
                    result = sraWithoutCast(lhs, rhs);
                    break;
                case common::Opcode::MUL:
                    result = lhs * rhs;
                    break;
                case common::Opcode::UDIV:
                    if (rhs == 0) [[unlikely]] {
                        throwUDIVDivByZero();
                    }
                    result = lhs / rhs;
                    break;
                case common::Opcode::SDIV: {
                    const auto signedLhs = static_cast<std::int32_t>(lhs);
                    const auto signedRhs = static_cast<std::int32_t>(rhs);

                    if (signedRhs == 0) [[unlikely]] {
                        throwSDIVDivByZero();
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
                    result = signExtend(source, instruction.c);
                    break;
                case common::Opcode::BSWAP:
                    result = byteSwap(source, instruction.c);
                    break;
                default:
                    throw std::logic_error("unreachable opcode in conversion block");
            }

            writeRegister(instruction.a, result);
            advancePc();
            return;
        }
        case common::Opcode::SCRW: {
            validateRegisterIndex(instruction.b, "source");

            const auto statusRegister = static_cast<common::StatusRegister>(instruction.a);
            const std::uint32_t value = readRegister(instruction.b);
            writeStatusRegister(statusRegister, value);
            if (statusRegister == common::StatusRegister::SpTop) {
                stackBottom_ = value;
            }
            advancePc();
            return;
        }
        case common::Opcode::SCRR: {
            validateRegisterIndex(instruction.a, "destination");

            const auto statusRegister = static_cast<common::StatusRegister>(instruction.b);
            writeRegister(instruction.a, readStatusRegister(statusRegister));
            advancePc();
            return;
        }
        case common::Opcode::ASPI:
            // signExtendBits строит 32-битное представление, которое затем интерпретируется как int32_t
            advanceStackPointer(
                instruction.a,
                static_cast<std::int32_t>(signExtendBits(instruction.imm, 16))
            );
            advancePc();
            return;
        case common::Opcode::ASPR: {
            validateRegisterIndex(instruction.b, "source");
            // значение регистра уже имеет ширину 32 бита, поэтому дополнительное расширение не требуется
            const std::int64_t delta = static_cast<std::int32_t>(readRegister(instruction.b));
            advanceStackPointer(instruction.a, delta);
            advancePc();
            return;
        }
        default:
            throwInvalidOpcode();
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
    if (emitRamWriteTrace) [[unlikely]] {
        // на каждом шаге буфер должен содержать только записи текущей инструкции
        pendingRamWriteTrace_.clear();
    }
    const bool emitDisasmTrace =
        traceOutput_ != nullptr &&
        tickRangeFilter_.contains(currentTick);

    TraceSnapshot before{};
    if (emitDisasmTrace) [[unlikely]] {
        // источники должны выводиться со значениями до исполнения команды
        before = captureTraceSnapshot(instruction);
    }
#endif

    execute(instruction);

    // счётчик увеличивается только после успешного выполнения инструкции
    ++tick_;

#if MCST_TRACING
    if (emitDisasmTrace) [[unlikely]] {
        writeDisasmTrace(
            currentTick,
            instructionAddress - programBase_,
            word,
            instruction,
            before
        );
    }
    if (emitRamWriteTrace) [[unlikely]] {
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
