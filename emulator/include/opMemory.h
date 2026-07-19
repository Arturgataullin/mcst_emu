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

    static constexpr std::size_t cellSize = 4;
    static_assert(cellSize > 0);
    static_assert((cellSize & (cellSize - 1)) == 0, "cellSize must be power of two");
    static_assert(blockSize % cellSize == 0, "blockSize must be a multiple of cellSize");

    static constexpr std::size_t cellsPerBlock = blockSize / cellSize;

    static_assert(cellsPerBlock > 0);
    static_assert((cellsPerBlock & (cellsPerBlock - 1)) == 0, "cellsPerBlock must be power of two");

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

    [[nodiscard]] std::uint8_t read8(std::uint32_t address) const;
    [[nodiscard]] std::uint16_t read16(std::uint32_t address) const;
    [[nodiscard]] std::uint32_t read32(std::uint32_t address) const;
    [[nodiscard]] std::uint32_t readInstruction32(std::uint32_t address) const;

    void write8(std::uint32_t address, std::uint8_t value);
    void write16(std::uint32_t address, std::uint16_t value);
    void write32(std::uint32_t address, std::uint32_t value);

    [[nodiscard]] std::size_t size() const noexcept;
    void setCellWriteHandler(CellWriteHandler handler);
    void setUninitReadHandler(UninitReadHandler handler);
    void clearRange(std::uint32_t address, std::size_t byteCount);
    void clearAndMarkUninitialized(std::uint32_t address, std::size_t byteCount);
    void markUninitialized(std::uint32_t address, std::size_t byteCount);

private:

    struct Block {
        std::array<std::uint32_t, cellsPerBlock> cells{};
        std::bitset<blockSize> initialized{};
    };

    // функции без проверки доступности памяти по адресу для использования в write/read16,32
    [[nodiscard]] std::uint8_t read8Unchecked(std::uint32_t address) const;
    void write8Unchecked(std::uint32_t address, std::uint8_t value);

    void writeUnchecked(std::uint32_t address, std::uint32_t value, std::size_t byteCount);
    // уведомляет только о финальном изменении ячейки, а не о каждом записанном байте
    void notifyCellWrites(
        std::size_t firstCellIndex,
        std::uint32_t firstOldData,
        std::size_t lastCellIndex,
        std::uint32_t lastOldData
    );

    void checkRange(std::uint32_t address, std::size_t size) const;
    void clearRangeImpl(std::uint32_t address, std::size_t byteCount, bool resetInitialized);
    void markInitialized(std::uint32_t address, std::size_t byteCount);
    [[nodiscard]] std::uint8_t getUninitMask(std::uint32_t address, std::size_t byteCount) const;
    void notifyUninitRead(std::uint32_t address, std::size_t byteCount) const;
    [[nodiscard]] std::uint32_t readCellUnchecked(std::size_t cellIndex) const;
    void writeCellUnchecked(std::size_t cellIndex, std::uint32_t value);
    [[nodiscard]] Block& getOrCreateBlock(std::uint32_t blockIndex);
    [[nodiscard]] const Block* findBlock(std::uint32_t blockIndex) const;
    void resetBlockCache() noexcept;
    [[nodiscard]] static std::uint32_t blockIndexForAddress(std::uint32_t address) noexcept;
    [[nodiscard]] static std::size_t blockOffsetForAddress(std::uint32_t address) noexcept;

    // эти функции работают с уже найденным блоком, чтобы не делать поиск в unordered_map на каждый байт
    [[nodiscard]] static std::uint8_t readByteFromBlock(const Block& block, std::size_t blockOffset) noexcept;
    static void writeByteToBlock(Block& block, std::size_t blockOffset, std::uint8_t value) noexcept;
    [[nodiscard]] static std::uint32_t byteRangeMask(std::size_t byteOffset, std::size_t byteCount) noexcept;
    static void clearBytesInCell(std::uint32_t& cell, std::size_t byteOffset, std::size_t byteCount) noexcept;
    static void clearBytesInBlock(Block& block, std::size_t blockOffset, std::size_t byteCount) noexcept;
    static void markInitializedInBlock(Block& block, std::size_t blockOffset, std::size_t byteCount);
    static void resetInitializedInBlock(Block& block, std::size_t blockOffset, std::size_t byteCount);

private:
    // память хранится блоками по 4кб, чтобы большой объем RAM не выделялся сразу
    std::unordered_map<std::uint32_t, Block> blocks_;
    std::size_t size_;
    CellWriteHandler cellWriteHandler_;
    UninitReadHandler cellReadHandler_;
    // кэши ускоряют повторные обращения в один и тот же 4 КБ блок
    mutable std::uint32_t cachedReadBlockIndex_ = 0;
    mutable const Block* cachedReadBlock_ = nullptr;
    std::uint32_t cachedWriteBlockIndex_ = 0;
    Block* cachedWriteBlock_ = nullptr;
};

}
