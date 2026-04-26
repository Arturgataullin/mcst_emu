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
    memory_.clear();

    memory_.load(program, baseAddress);

    programBase_ = baseAddress;
    programEnd_ = baseAddress + static_cast<std::uint32_t>(program.size());
    pc_ = baseAddress;
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
    if (reg >= common::registerCount) {
        throw std::runtime_error(std::string("invalid register index in ") + fieldName);
    }
}

void Emulator::execute(const DecodedInstruction& instruction) {
    switch (instruction.opcode) {
        case common::Opcode::LI: {
            validateRegisterIndex(instruction.a, "LI destination");
            registers_[instruction.a] = instruction.imm;
            pc_ += common::instructionSizeBytes;
            return;
        }

        case common::Opcode::ADD: {
            validateRegisterIndex(instruction.a, "ADD destination");
            validateRegisterIndex(instruction.b, "ADD left source");
            validateRegisterIndex(instruction.c, "ADD right source");

            registers_[instruction.a] = registers_[instruction.b] + registers_[instruction.c];
            pc_ += common::instructionSizeBytes;
            return;
        }
    }

    throw std::runtime_error("invalid opcode");
}

void Emulator::step() {
    if (isFinished()) {
        return;
    }

    const std::uint32_t word = fetchInstructionWord();
    const DecodedInstruction instruction = decode(word);
    execute(instruction);
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