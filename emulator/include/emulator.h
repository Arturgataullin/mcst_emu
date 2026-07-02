#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "isa.h"
#include "opMemory.h"

namespace emulator {

struct DecodedInstruction {
    common::Opcode opcode{};
    std::uint8_t a = 0;
    std::uint8_t b = 0;
    std::uint8_t c = 0;
    std::uint16_t imm = 0;
};

class Emulator {
public:
    void loadProgramFromFile(const std::string& inputPath);
    void loadProgram(const std::vector<std::uint8_t>& program,
                     std::uint32_t baseAddress = common::resetAddress);

    void run();
    void step();

    const std::array<std::uint32_t, common::registerCount>& registers() const noexcept;
    std::uint32_t pc() const noexcept;
    bool isFinished() const noexcept;

    std::string dumpRegisters() const;

private:
    std::uint32_t fetchInstructionWord() const;
    DecodedInstruction decode(std::uint32_t word) const;
    void execute(const DecodedInstruction& instruction);

    static std::vector<std::uint8_t> readBinaryFile(const std::string& inputPath);
    void validateRegisterIndex(std::uint8_t reg, const char* fieldName) const;
    std::uint32_t readRegister(std::uint8_t reg) const;
    void writeRegister(std::uint8_t reg, std::uint32_t value);

private:
    std::array<std::uint32_t, common::registerCount> registers_{};
    std::uint32_t assemblerTempRegister_ = 0;
    Memory memory_;

    std::uint32_t pc_ = common::resetAddress;
    std::uint32_t programBase_ = common::resetAddress;
    std::uint32_t programEnd_ = common::resetAddress;
};

}
