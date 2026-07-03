#include "emulator.h"

#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <stdexcept>

namespace emulator {

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

    registers_.fill(0);
    assemblerTempRegister_ = 0;
    memory_.clear();

    memory_.load(program, baseAddress);

    programBase_ = baseAddress;
    programEnd_ = baseAddress + static_cast<std::uint32_t>(program.size());
    pc_ = baseAddress;

#if MCST_TRACING
    tick_ = 0;
#endif
}

const std::array<std::uint32_t, common::registerCount>& Emulator::registers() const noexcept {
    return registers_;
}

std::uint32_t Emulator::pc() const noexcept {
    return pc_;
}

bool Emulator::isFinished() const noexcept {
    return pc_ >= programEnd_;
}

std::uint32_t Emulator::fetchInstructionWord() const {
    if (pc_ + common::instructionSizeBytes > programEnd_) {
        throw std::runtime_error("instruction fetch goes past loaded program");
    }

    return memory_.read32(pc_);
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

// доп задание для 2.1
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
        default:
            throw std::runtime_error("invalid opcode");
    }
}

void Emulator::step() {
    if (isFinished()) {
        return;
    }

    const std::uint32_t instructionAddress = pc_;
    const std::uint32_t word = fetchInstructionWord();
    const DecodedInstruction instruction = decode(word);

#if MCST_TRACING
    // номер такта равен числу команд, исполненных перед текущей командой
    const std::uint64_t currentTick = tick_;
    const bool emitTrace =
        traceOutput_ != nullptr &&
        containsTick(traceRanges_, currentTick);

    StateSnapshot before{};
    if (emitTrace) {
        // источники должны выводиться со значениями до исполнения команды
        before.registers = registers_;
        before.tempRegister = assemblerTempRegister_;
    }
#endif

    execute(instruction);

#if MCST_TRACING
    // счётчик увеличивается только после успешного исполнения команды
    ++tick_;
    if (emitTrace) {
        writeDisasmTrace(
            currentTick,
            instructionAddress - programBase_,
            word,
            instruction,
            before
        );
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
