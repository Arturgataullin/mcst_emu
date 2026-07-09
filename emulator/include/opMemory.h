#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "isa.h"

namespace emulator {

class Memory {
public:
    explicit Memory(std::size_t size = common::opMemorySize);

    void clear() noexcept;

    void load(const std::vector<std::uint8_t>& bytes, std::uint32_t baseAddress = common::resetAddress);

    std::uint8_t read8(std::uint32_t address) const;
    std::uint16_t read16(std::uint32_t address) const;
    std::uint32_t read32(std::uint32_t address) const;

    void write8(std::uint32_t address, std::uint8_t value);
    void write16(std::uint32_t address, std::uint16_t value);
    void write32(std::uint32_t address, std::uint32_t value);

    std::size_t size() const noexcept;

private:
    // функции без проверки доступности памти по адресу для использования в write/read16,32
    std::uint8_t read8Unchecked(std::uint32_t address) const;
    void write8Unchecked(std::uint32_t address, std::uint8_t value);

    void checkAddress(std::uint32_t address) const;
    void checkRange(std::uint32_t address, std::size_t size) const;

private:
    std::vector<std::uint32_t> cells_;
    std::size_t size_;
};

}
