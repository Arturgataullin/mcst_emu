#pragma once

#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

#include "isa.h"

namespace emulator {

class Memory {
public:
    static constexpr std::size_t blockSize = 4096;
    static_assert(blockSize > 0);
    static_assert((blockSize & (blockSize - 1)) == 0, "blockSize must be power of two");

    // обработчик нужен для трассировки изменения 4-байтовых ячеек памяти
    using CellWriteHandler = std::function<void(
        std::uint32_t cellAddress,
        std::uint32_t oldData,
        std::uint32_t newData
    )>;

    // обработчик сообщает только факт чтения неинициализированных байтов, формат предупреждения остается в Emulator
    using UninitReadHandler = std::function<void(
        std::uint32_t address,
        std::size_t size,
        std::uint8_t uninitMask
    )>;

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
    void setCellWriteHandler(CellWriteHandler handler);
    void setUninitReadHandler(UninitReadHandler handler);

private:
    static constexpr std::size_t cellSize = 4;
    static_assert(cellSize > 0);
    static_assert((cellSize & (cellSize - 1)) == 0, "cellSize must be power of two");
    static_assert(blockSize % cellSize == 0, "blockSize must be a multiple of cellSize");

    static constexpr std::size_t cellsPerBlock = blockSize / cellSize;

    static_assert(cellsPerBlock > 0);
    static_assert((cellsPerBlock & (cellsPerBlock - 1)) == 0, "cellsPerBlock must be power of two");

    struct Block {
        std::array<std::uint32_t, cellsPerBlock> cells{};
        std::bitset<blockSize> initialized{};
    };

    // функции без проверки доступности памти по адресу для использования в write/read16,32
    std::uint8_t read8Unchecked(std::uint32_t address) const;
    void write8Unchecked(std::uint32_t address, std::uint8_t value);

    void writeUnchecked(std::uint32_t address, std::uint32_t value, std::size_t byteCount);
    // уведомляет только о финальном изменении ячейки, а не о каждом записанном байте
    void notifyCellWrites(
        std::size_t firstCellIndex,
        std::uint32_t firstOldData,
        std::size_t lastCellIndex,
        std::uint32_t lastOldData
    );

    void checkAddress(std::uint32_t address) const;
    void checkRange(std::uint32_t address, std::size_t size) const;
    void markInitialized(std::uint32_t address, std::size_t byteCount);
    std::uint8_t getUninitMask(std::uint32_t address, std::size_t byteCount) const;
    void notifyUninitRead(std::uint32_t address, std::size_t byteCount) const;
    std::uint32_t readCellUnchecked(std::size_t cellIndex) const;
    Block& getOrCreateBlock(std::uint32_t blockIndex);
    const Block* findBlock(std::uint32_t blockIndex) const;
    static std::uint32_t blockIndexForAddress(std::uint32_t address) noexcept;
    static std::size_t blockOffsetForAddress(std::uint32_t address) noexcept;

private:
    // память хранится блоками по 4кб, чтобы большой объем RAM не выделялся сразу
    std::unordered_map<std::uint32_t, Block> blocks_;
    std::size_t size_;
    CellWriteHandler cellWriteHandler_;
    UninitReadHandler cellReadHandler_;
};

}
