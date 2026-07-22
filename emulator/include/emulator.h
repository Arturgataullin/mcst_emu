#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

#include "isa.h"
#include "opMemory.h"

// при сборке вне CMake трассировка по умолчанию включена
#ifndef MCST_TRACING
#define MCST_TRACING 1
#endif

#if MCST_TRACING
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

    [[nodiscard]] const std::array<std::uint32_t, common::registerCount>& registers() const noexcept;
    [[nodiscard]] std::uint64_t pc() const noexcept;
    [[nodiscard]] bool isFinished() const noexcept;

    [[nodiscard]] std::string dumpRegisters() const;
    void enableUninitializedRamWarnings(std::ostream& output);
    void enableClearStack();

#if MCST_TRACING
    // void enableDisasmTrace(std::ostream& output, std::vector<TickRange> ranges = {});

    void enableDisasmTrace(std::ostream& output);
    void setTraceTicks(std::vector<TickRange> ranges);
    void enableRamWriteTrace(std::ostream& output, std::vector<AddressRange> ranges = {});
#endif

private:
    [[nodiscard]] std::uint32_t fetchInstructionWord() const;
    [[nodiscard]] DecodedInstruction decode(std::uint32_t word) const;
    void execute(const DecodedInstruction& instruction);

    [[nodiscard]] static std::vector<std::uint8_t> readBinaryFile(const std::string& inputPath);
    void validateRegisterIndex(std::uint8_t reg, const char* fieldName) const;
    [[nodiscard]] std::uint32_t readRegister(std::uint8_t reg) const;
    void writeRegister(std::uint8_t reg, std::uint32_t value);
    void writeUninitRamWarning(std::uint32_t address, std::size_t byteCount, std::uint8_t uninitMask) const;

    [[nodiscard]] std::uint32_t readStatusRegister(std::uint8_t index) const;
    void writeStatusRegister(std::uint8_t index, std::uint32_t value);
    void advanceStackPointer(std::uint8_t destination, std::int64_t delta);

    // вызывается только после проверки !isFinished(); run() выбирает trace-ветку один раз до цикла
    template <bool TraceEnabled>
    void stepImpl();

#if MCST_TRACING
    // снимок хранит только значения операндов текущей инструкции, а не весь файл регистров
    struct TraceSnapshot {
        std::uint32_t a = 0;
        std::uint32_t b = 0;
        std::uint32_t c = 0;
    };

    void writeDisasmTrace(
        std::uint64_t tick,
        std::uint32_t address,
        std::uint32_t encoding,
        const DecodedInstruction& instruction,
        const TraceSnapshot& before
    ) const;

    struct RamWriteTraceEvent {
        std::uint64_t tick = 0;
        std::uint32_t cellAddress = 0;
        std::uint32_t oldData = 0;
        std::uint32_t newData = 0;
    };

    [[nodiscard]] TraceSnapshot captureTraceSnapshot(const DecodedInstruction& instruction) const;
    void collectRamWriteTrace(std::uint32_t cellAddress, std::uint32_t oldData, std::uint32_t newData);
    void writeRamWriteTrace(const RamWriteTraceEvent& event) const;
    void flushRamWriteTrace();
#endif

private:
    std::array<std::uint32_t, common::registerCount> registers_{};
    std::uint32_t assemblerTempRegister_ = 0;
    // исходное значение SP_TOP ограничивает освобождение стека сверху
    std::uint32_t stackBottom_ = 0;
    // индекс элемента совпадает с индексом SCR в машинной инструкции
    std::array<std::uint32_t, common::statusRegisterCount> statusRegisters_{};

    Memory memory_;

    std::uint64_t pc_ = common::resetAddress;
    std::uint64_t programBase_ = common::resetAddress;
    std::uint64_t programEnd_ = common::resetAddress;
    std::uint64_t tick_ = 0;
    bool clearStack_ = false;
    std::ostream* uninitRamWarningOutput_ = nullptr;

#if MCST_TRACING
    std::ostream* traceOutput_ = nullptr;
    TickRangeFilter tickRangeFilter_;
    std::ostream* ramWriteTraceOutput_ = nullptr;
    AddressRangeFilter ramWriteAddressFilter_;
    // события RAM write копятся во время execute(), а печатаются после завершения инструкции
    std::vector<RamWriteTraceEvent> pendingRamWriteTrace_;
#endif
};

}
