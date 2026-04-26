#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "isa.h"

namespace emulator {

class Memory {
public:
    void clear() noexcept;

    void load(const std::vector<std::uint8_t>& bytes, std::uint32_t baseAddress = common::resetAddress);

    std::uint32_t read32(std::uint32_t address) const;
    std::uint8_t read8(std::uint32_t address) const;

    void write8(std::uint32_t address, std::uint8_t value);

private:
    void checkAddress(std::uint32_t address) const;
    void checkRange(std::uint32_t address, std::size_t size) const;

private:
    std::array<std::uint8_t, common::opMemorySize> data_{};
};

}