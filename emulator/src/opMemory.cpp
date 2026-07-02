#include "opMemory.h"

#include <stdexcept>

namespace emulator {

void Memory::clear() noexcept {
    data_.fill(0);
}

void Memory::load(const std::vector<std::uint8_t>& bytes, std::uint32_t baseAddress) {
    checkRange(baseAddress, bytes.size());

    for (std::size_t i = 0; i < bytes.size(); ++i) {
        data_[baseAddress + i] = bytes[i];
    }
}

std::uint32_t Memory::read32(std::uint32_t address) const {
    checkRange(address, 4);

    std::uint32_t value = 0;
    value |= static_cast<std::uint32_t>(data_[address]);
    value |= static_cast<std::uint32_t>(data_[address + 1]) << 8;
    value |= static_cast<std::uint32_t>(data_[address + 2]) << 16;
    value |= static_cast<std::uint32_t>(data_[address + 3]) << 24;
    return value;
}

std::uint8_t Memory::read8(std::uint32_t address) const {
    checkRange(address, 1);
    return data_[address];
}

void Memory::write8(std::uint32_t address, std::uint8_t value) {
    checkAddress(address);
    data_[address] = value;
}

void Memory::checkAddress(std::uint32_t address) const {
    if (address >= data_.size()) {
        throw std::runtime_error("memory access out of range");
    }
}

void Memory::checkRange(std::uint32_t address, std::size_t size) const {
    if (address + size > data_.size()) { // переполнение мб
        throw std::runtime_error("memory range access out of range");
    }
}

} 