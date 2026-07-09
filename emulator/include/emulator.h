#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "isa.h"
#include "opMemory.h"

// при сборке вне CMake трассировка по умолчанию включена
#ifndef MCST_TRACING
#define MCST_TRACING 1
#endif

#if MCST_TRACING
#include <iosfwd>
#include "ranges.h"
#endif

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
    explicit Emulator(std::size_t memorySize = common::opMemorySize);

    void loadProgramFromFile(const std::string& inputPath);
    void loadProgram(const std::vector<std::uint8_t>& program,
                     std::uint32_t baseAddress = common::resetAddress);

    void run();
    void step();

    const std::array<std::uint32_t, common::registerCount>& registers() const noexcept;
    std::uint32_t pc() const noexcept;
    bool isFinished() const noexcept;

    std::string dumpRegisters() const;

#if MCST_TRACING
    // void enableDisasmTrace(std::ostream& output, std::vector<TickRange> ranges = {});

    void enableDisasmTrace(std::ostream& output);
    void setTraceTicks(std::vector<TickRange> ranges);
    void enableRamWriteTrace(std::ostream& output, std::vector<AddressRange> ranges = {});
#endif

private:
    std::uint32_t fetchInstructionWord() const;
    DecodedInstruction decode(std::uint32_t word) const;
    void execute(const DecodedInstruction& instruction);

    static std::vector<std::uint8_t> readBinaryFile(const std::string& inputPath);
    void validateRegisterIndex(std::uint8_t reg, const char* fieldName) const;
    std::uint32_t readRegister(std::uint8_t reg) const;
    void writeRegister(std::uint8_t reg, std::uint32_t value);

#if MCST_TRACING
    // снимок нужен для вывода исходных значений регистров-источников
    struct StateSnapshot {
        std::array<std::uint32_t, common::registerCount> registers{};
        std::uint32_t tempRegister = 0;
    };

    void writeDisasmTrace(
        std::uint64_t tick,
        std::uint32_t address,
        std::uint32_t encoding,
        const DecodedInstruction& instruction,
        const StateSnapshot& before
    ) const;

    struct RamWriteTraceEvent {
        std::uint64_t tick = 0;
        std::uint32_t cellAddress = 0;
        std::uint32_t oldData = 0;
        std::uint32_t newData = 0;
    };

    void collectRamWriteTrace(std::uint32_t cellAddress, std::uint32_t oldData, std::uint32_t newData);
    void writeRamWriteTrace(const RamWriteTraceEvent& event) const;
    void flushRamWriteTrace();
#endif

private:
    std::array<std::uint32_t, common::registerCount> registers_{};
    std::uint32_t assemblerTempRegister_ = 0;
    Memory memory_;

    std::uint32_t pc_ = common::resetAddress;
    std::uint32_t programBase_ = common::resetAddress;
    std::uint32_t programEnd_ = common::resetAddress;

#if MCST_TRACING
    std::ostream* traceOutput_ = nullptr;
    TickRangeFilter tickRangeFilter_;
    std::ostream* ramWriteTraceOutput_ = nullptr;
    AddressRangeFilter ramWriteAddressFilter_;
    // события RAM write копятся во время execute(), а печатаются после завершения инструкции
    std::vector<RamWriteTraceEvent> pendingRamWriteTrace_;
    std::uint64_t tick_ = 0;
#endif
};

}
