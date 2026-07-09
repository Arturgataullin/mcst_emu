#include "opMemory.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace emulator {

constexpr std::size_t cellSize = 4;
constexpr std::size_t cellMask = 3;
constexpr std::size_t cellShift = 2;
constexpr std::uint32_t byteMask = 0xffu;

Memory::Memory(std::size_t size) {
    constexpr std::size_t maxAddressableSize =
        static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) + 1u; // общее количество количество адресуемых байт

    if (size == 0) {
        throw std::runtime_error("memory size must be positive");
    }

    if (size % cellSize != 0) {
        throw std::runtime_error("memory size must be a multiple of 4 bytes");
    }

    if (size > maxAddressableSize) {
        throw std::runtime_error("memory size exceeds 32-bit address space");
    }

    size_ = size;
    cells_.assign(size_ / cellSize, 0);
}

void Memory::clear() noexcept {
    std::fill(cells_.begin(), cells_.end(), 0);
}

// загружает bytes в память, начиная с baseAddress включительно
void Memory::load(const std::vector<std::uint8_t>& bytes, std::uint32_t baseAddress) {
    checkRange(baseAddress, bytes.size());

    for (std::size_t i = 0; i < bytes.size(); ++i) {
        write8(baseAddress + static_cast<std::uint32_t>(i), bytes[i]);
    }
}

std::uint8_t Memory::read8Unchecked(std::uint32_t address) const {
    const std::size_t cellIndex = address >> cellShift; // заменяю / 4 на более быстрый >> 2
    const std::size_t byteIndex = address & cellMask;
    const std::size_t shift = byteIndex * 8; // количиство сдвигов вправо для получения необходимого байта (потом маскирую 0xff)

    return static_cast<std::uint8_t>((cells_[cellIndex] >> shift) & byteMask);
}

void Memory::write8Unchecked(std::uint32_t address, std::uint8_t value) {
    const std::size_t cellIndex = address / cellSize;
    const std::size_t byteIndex = address & cellMask;
    const std::size_t shift = byteIndex * 8;
    const std::uint32_t mask = byteMask << shift; // с помощью этой маски обнуляю байт, который нужно заменить

    cells_[cellIndex] = (cells_[cellIndex] & ~mask) |
                        (static_cast<std::uint32_t>(value) << shift);
}

std::uint8_t Memory::read8(std::uint32_t address) const {
    checkRange(address, 1);
    return read8Unchecked(address);
}

std::uint16_t Memory::read16(std::uint32_t address) const {
    checkRange(address, 2);

    std::uint16_t value = 0;
    value |= static_cast<std::uint16_t>(read8Unchecked(address));
    value |= static_cast<std::uint16_t>(read8Unchecked(address + 1)) << 8;
    return value;
}

std::uint32_t Memory::read32(std::uint32_t address) const {
    checkRange(address, 4);

    std::uint32_t value = 0;
    value |= static_cast<std::uint32_t>(read8Unchecked(address));
    value |= static_cast<std::uint32_t>(read8Unchecked(address + 1)) << 8;
    value |= static_cast<std::uint32_t>(read8Unchecked(address + 2)) << 16;
    value |= static_cast<std::uint32_t>(read8Unchecked(address + 3)) << 24;
    return value;
}

// запись 0xAB по адресу 0x0
// как выглядит ячейка (4 байта): 0x000000AB
void Memory::write8(std::uint32_t address, std::uint8_t value) {
    checkAddress(address);
    write8Unchecked(address, value);
}

void Memory::write16(std::uint32_t address, std::uint16_t value) {
    checkRange(address, 2);

    write8Unchecked(address, static_cast<std::uint8_t>(value & byteMask));
    write8Unchecked(address + 1, static_cast<std::uint8_t>((value >> 8) & byteMask));
}

void Memory::write32(std::uint32_t address, std::uint32_t value) {
    checkRange(address, 4);

    write8Unchecked(address, static_cast<std::uint8_t>(value & byteMask));
    write8Unchecked(address + 1, static_cast<std::uint8_t>((value >> 8) & byteMask));
    write8Unchecked(address + 2, static_cast<std::uint8_t>((value >> 16) & byteMask));
    write8Unchecked(address + 3, static_cast<std::uint8_t>((value >> 24) & byteMask));
}

std::size_t Memory::size() const noexcept {
    return size_;
}

void Memory::checkAddress(std::uint32_t address) const {
    checkRange(address, 1);
}

void Memory::checkRange(std::uint32_t address, std::size_t accessSize) const {
    if (accessSize > size_ || static_cast<std::size_t>(address) > size_ - accessSize) [[unlikely]] {
        throw std::runtime_error("memory range access out of range");
    }
}

}
