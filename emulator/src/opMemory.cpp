#include "opMemory.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace emulator {

namespace {

[[noreturn]] void throwMemoryRangeAccessOutOfRange() {
    throw std::runtime_error("memory range access out of range");
}

}

// снаружи память адресуется байтами, внутри каждый выделенный блок хранит массив 4-байтовых ячеек

constexpr std::size_t cellMask = 3;
constexpr std::size_t cellShift = 2;
constexpr std::size_t blockShift = 12;
// cellsPerBlock тоже степень двойки, поэтому номер блока по номеру ячейки можно получить сдвигом
constexpr std::size_t cellsPerBlockShift = blockShift - cellShift;
constexpr std::size_t blockMask = Memory::blockSize - 1;
constexpr std::uint32_t byteMask = 0xffu;

static_assert((Memory::cellSize & (Memory::cellSize - 1)) == 0, "cellSize must be power of two");
static_assert(cellMask == Memory::cellSize - 1);
static_assert((std::size_t{1} << cellShift) == Memory::cellSize);
static_assert((Memory::blockSize & (Memory::blockSize - 1)) == 0, "blockSize must be power of two");
static_assert(blockMask == Memory::blockSize - 1);
static_assert((std::size_t{1} << blockShift) == Memory::blockSize);
static_assert((std::size_t{1} << cellsPerBlockShift) == Memory::cellsPerBlock);

Memory::Memory(std::size_t size) {
    constexpr std::size_t maxAddressableSize =
        static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) + 1u; // общее количество адресуемых байт

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
}

void Memory::clear() noexcept {
    // при очистке достаточно удалить выделенные блоки, не проходя по всему размеру RAM
    blocks_.clear();
    resetBlockCache();
}

// загружает bytes в память, начиная с baseAddress включительно
void Memory::load(const std::vector<std::uint8_t>& bytes, std::uint32_t baseAddress) {
    checkRange(baseAddress, bytes.size());

    std::size_t loaded = 0;
    while (loaded < bytes.size()) {
        // загрузка идет кусками внутри одного блока, чтобы не искать unordered_map для каждого байта
        const std::uint32_t currentAddress = baseAddress + static_cast<std::uint32_t>(loaded);
        const std::size_t blockOffset = blockOffsetForAddress(currentAddress);
        const std::size_t bytesLeft = bytes.size() - loaded;
        const std::size_t bytesInBlock =
            bytesLeft < blockSize - blockOffset ? bytesLeft : blockSize - blockOffset;

        Block& block = getOrCreateBlock(blockIndexForAddress(currentAddress));
        for (std::size_t i = 0; i < bytesInBlock; ++i) {
            // load не считается исполняемой записью в ram, поэтому callback трассировки здесь не вызывается
            const std::size_t currentOffset = blockOffset + i;
            writeByteToBlock(block, currentOffset, bytes[loaded + i]);
            block.initialized.set(currentOffset);
        }

        loaded += bytesInBlock;
    }
}

std::uint8_t Memory::read8Unchecked(std::uint32_t address) const {
    const Block* block = findBlock(blockIndexForAddress(address));
    if (block == nullptr) {
        // невыделенный блок читается как нули, но его байты остаются неинициализированными
        return 0;
    }

    return readByteFromBlock(*block, blockOffsetForAddress(address));
}

void Memory::write8Unchecked(std::uint32_t address, std::uint8_t value) {
    Block& block = getOrCreateBlock(blockIndexForAddress(address));
    writeByteToBlock(block, blockOffsetForAddress(address), value);
}

void Memory::writeUnchecked(std::uint32_t address, std::uint32_t value, std::size_t byteCount) {
    for (std::size_t i = 0; i < byteCount; ++i) {
        // младший байт значения попадает в меньший адрес
        const std::uint8_t byteValue =
            static_cast<std::uint8_t>((value >> (i * 8)) & byteMask);
        write8Unchecked(address + static_cast<std::uint32_t>(i), byteValue);
    }
}

void Memory::notifyCellWrites(
    std::size_t firstCellIndex,
    std::uint32_t firstOldData,
    std::size_t lastCellIndex,
    std::uint32_t lastOldData
) {
    if (!cellWriteHandler_) [[likely]] {
        return;
    }

    // одна операция записи может задеть максимум две 4-байтовые ячейки
    cellWriteHandler_(
        static_cast<std::uint32_t>(firstCellIndex * cellSize),
        firstOldData,
        readCellUnchecked(firstCellIndex)
    );

    if (lastCellIndex != firstCellIndex) {
        cellWriteHandler_(
            static_cast<std::uint32_t>(lastCellIndex * cellSize),
            lastOldData,
            readCellUnchecked(lastCellIndex)
        );
    }
}

std::uint8_t Memory::read8(std::uint32_t address) const {
    checkRange(address, 1);
    notifyUninitRead(address, 1);
    return read8Unchecked(address);
}

std::uint16_t Memory::read16(std::uint32_t address) const {
    checkRange(address, 2);
    notifyUninitRead(address, 2);

    // многобайтовые операции собираются по байтам, поэтому работают и на невыровненных адресах
    std::uint16_t value = 0;
    value |= static_cast<std::uint16_t>(read8Unchecked(address));
    value |= static_cast<std::uint16_t>(read8Unchecked(address + 1)) << 8;
    return value;
}

std::uint32_t Memory::read32(std::uint32_t address) const {
    checkRange(address, 4);
    notifyUninitRead(address, 4);

    const std::size_t firstCellIndex = address >> cellShift;
    const std::size_t byteIndex = address & cellMask;
    const std::uint32_t firstCell = readCellUnchecked(firstCellIndex);

    if (byteIndex != 0) [[unlikely]] {
        // невыровненное чтение 32 бит всегда берет хвост текущей ячейки и начало следующей
        const std::uint32_t secondCell = readCellUnchecked(firstCellIndex + 1);
        const std::size_t firstShift = byteIndex * 8;
        const std::size_t secondShift = (cellSize - byteIndex) * 8;
        return (firstCell >> firstShift) | (secondCell << secondShift);
    }
    // выровненное чтение 32 бит совпадает с одной внутренней ячейкой памяти
    return firstCell;
}

// запись 0xAB по адресу 0x0
// как выглядит ячейка (4 байта): 0x000000AB
void Memory::write8(std::uint32_t address, std::uint8_t value) {
    checkRange(address, 1);

    const std::size_t cellIndex = address >> cellShift;
    const std::uint32_t oldData = readCellUnchecked(cellIndex);

    writeUnchecked(address, value, 1);
    markInitialized(address, 1);
    notifyCellWrites(cellIndex, oldData, cellIndex, oldData);
}

void Memory::write16(std::uint32_t address, std::uint16_t value) {
    checkRange(address, 2);

    // сохраняю старые значения до записи, чтобы trace видел old -> new
    const std::size_t firstCellIndex = address >> cellShift;
    const std::size_t lastCellIndex = (static_cast<std::size_t>(address) + 1) >> cellShift;
    const std::uint32_t firstOldData = readCellUnchecked(firstCellIndex);
    const std::uint32_t lastOldData = readCellUnchecked(lastCellIndex);

    writeUnchecked(address, value, 2);
    markInitialized(address, 2);
    notifyCellWrites(firstCellIndex, firstOldData, lastCellIndex, lastOldData);
}

void Memory::write32(std::uint32_t address, std::uint32_t value) {
    checkRange(address, 4);

    const std::size_t firstCellIndex = address >> cellShift;
    const std::size_t byteIndex = address & cellMask;

    if (byteIndex == 0) [[likely]] {
        // выровненная запись меняет одну ячейку, поэтому можно не раскладывать value по байтам
        const std::uint32_t blockIndex = blockIndexForAddress(address);
        const std::size_t blockOffset = blockOffsetForAddress(address);
        Block& block = getOrCreateBlock(blockIndex);
        const std::size_t cellIndexInBlock = blockOffset >> cellShift;
        // oldData нужен только для трассировки, без callback лишнее чтение старого значения не делаем
        const std::uint32_t firstOldData = cellWriteHandler_ ? block.cells[cellIndexInBlock] : 0;

        block.cells[cellIndexInBlock] = value;
        markInitializedInBlock(block, blockOffset, 4);

        if (cellWriteHandler_) {
            cellWriteHandler_(
                static_cast<std::uint32_t>(firstCellIndex * cellSize),
                firstOldData,
                value
            );
        }
        return;
    }

    // невыровненная запись 32 бит меняет две соседние 4-байтовые ячейки
    const std::size_t lastCellIndex = firstCellIndex + 1;
    const std::uint32_t firstOldData = readCellUnchecked(firstCellIndex);
    const std::uint32_t lastOldData = readCellUnchecked(lastCellIndex);

    const std::size_t firstShift = byteIndex * 8;
    const std::uint32_t firstMask = ~std::uint32_t{0} << firstShift;
    // первая маска сохраняет байты ячейки до адреса записи и заменяет хвост ячейки
    const std::uint32_t firstNewData =
        (firstOldData & ~firstMask) | ((value << firstShift) & firstMask);

    const std::size_t firstByteCount = cellSize - byteIndex;
    const std::size_t secondByteCount = byteIndex;
    const std::uint32_t secondMask =
        (std::uint32_t{1} << (secondByteCount * 8)) - 1u;
    // вторая маска заменяет начало следующей ячейки оставшимися байтами value
    const std::uint32_t secondNewData =
        (lastOldData & ~secondMask) | ((value >> (firstByteCount * 8)) & secondMask);

    writeCellUnchecked(firstCellIndex, firstNewData);
    writeCellUnchecked(lastCellIndex, secondNewData);
    markInitialized(address, 4);
    if (cellWriteHandler_) {
        cellWriteHandler_(
            static_cast<std::uint32_t>(firstCellIndex * cellSize),
            firstOldData,
            firstNewData
        );
        cellWriteHandler_(
            static_cast<std::uint32_t>(lastCellIndex * cellSize),
            lastOldData,
            secondNewData
        );
    }
}

std::size_t Memory::size() const noexcept {
    return size_;
}

void Memory::setCellWriteHandler(CellWriteHandler handler) {
    // пустой handler отключает уведомления о записи
    cellWriteHandler_ = std::move(handler);
}

void Memory::setUninitReadHandler(UninitReadHandler handler) {
    // пустой handler отключает уведомления о чтении неинициализированных байтов
    cellReadHandler_ = std::move(handler);
}

void Memory::clearRange(std::uint32_t address, std::size_t byteCount) {
    clearRangeImpl(address, byteCount, false);
}

void Memory::clearAndMarkUninitialized(std::uint32_t address, std::size_t byteCount) {
    clearRangeImpl(address, byteCount, true);
}

void Memory::markUninitialized(std::uint32_t address, std::size_t byteCount) {
    checkRange(address, byteCount);

    if (byteCount == 0) {
        return;
    }

    std::size_t marked = 0;
    while (marked < byteCount) {
        const std::uint32_t currentAddress = address + static_cast<std::uint32_t>(marked);
        const std::size_t blockOffset = blockOffsetForAddress(currentAddress);
        const std::size_t bytesInBlock = std::min(
            byteCount - marked,
            blockSize - blockOffset
        );
        const auto blockIt = blocks_.find(blockIndexForAddress(currentAddress));

        // байты отсутствующего разреженного блока уже считаются неинициализированными
        if (blockIt != blocks_.end()) {
            resetInitializedInBlock(blockIt->second, blockOffset, bytesInBlock);
        }

        marked += bytesInBlock;
    }
}

void Memory::checkRange(std::uint32_t address, std::size_t accessSize) const {
    // не считаем address + accessSize напрямую, чтобы не получить переполнение
    if (accessSize > size_ || static_cast<std::size_t>(address) > size_ - accessSize) [[unlikely]] {
        throwMemoryRangeAccessOutOfRange();
    }
}

void Memory::clearRangeImpl(std::uint32_t address, std::size_t byteCount, bool resetInitialized) {
    checkRange(address, byteCount);

    if (byteCount == 0) {
        return;
    }

    const bool traceWrites = static_cast<bool>(cellWriteHandler_);

    std::size_t cleared = 0;
    while (cleared < byteCount) {
        const std::uint32_t currentAddress = address + static_cast<std::uint32_t>(cleared);
        const std::uint32_t blockIndex = blockIndexForAddress(currentAddress);
        const std::size_t blockOffset = blockOffsetForAddress(currentAddress);
        const std::size_t bytesInBlock = std::min(
            byteCount - cleared,
            blockSize - blockOffset
        );
        const auto blockIt = blocks_.find(blockIndex);

        // отсутствующий разреженный блок уже содержит нули и считается неинициализированным
        if (blockIt == blocks_.end()) {
            cleared += bytesInBlock;
            continue;
        }

        Block& block = blockIt->second;
        if (!traceWrites) [[likely]] {
            clearBytesInBlock(block, blockOffset, bytesInBlock);
        }
        else {
            // RAM trace требует отдельного old -> new события для каждой затронутой ячейки
            std::size_t clearedInBlock = 0;
            while (clearedInBlock < bytesInBlock) {
                const std::size_t currentOffset = blockOffset + clearedInBlock;
                const std::size_t cellOffset = currentOffset & (cellSize - 1);
                const std::size_t bytesInCell = std::min(
                    bytesInBlock - clearedInBlock,
                    cellSize - cellOffset
                );
                const std::size_t cellIndexInBlock = currentOffset >> cellShift;
                const std::uint32_t oldData = block.cells[cellIndexInBlock];

                clearBytesInCell(block.cells[cellIndexInBlock], cellOffset, bytesInCell);

                cellWriteHandler_(
                    static_cast<std::uint32_t>(
                        (static_cast<std::size_t>(blockIndex) << blockShift) +
                        cellIndexInBlock * cellSize
                    ),
                    oldData,
                    block.cells[cellIndexInBlock]
                );
                clearedInBlock += bytesInCell;
            }
        }

        if (resetInitialized) {
            // при совместной очистке значения и признаки инициализации сбрасываются за один проход по блокам
            resetInitializedInBlock(block, blockOffset, bytesInBlock);
        }

        cleared += bytesInBlock;
    }
}

void Memory::markInitialized(std::uint32_t address, std::size_t byteCount) {
    std::size_t marked = 0;
    while (marked < byteCount) {
        // факт записи важнее значения: даже запись нуля делает байт инициализированным
        const std::uint32_t currentAddress = address + static_cast<std::uint32_t>(marked);
        const std::size_t blockOffset = blockOffsetForAddress(currentAddress);
        const std::size_t bytesLeft = byteCount - marked;
        const std::size_t bytesInBlock =
            bytesLeft < blockSize - blockOffset ? bytesLeft : blockSize - blockOffset;

        markInitializedInBlock(
            getOrCreateBlock(blockIndexForAddress(currentAddress)),
            blockOffset,
            bytesInBlock
        );

        marked += bytesInBlock;
    }
}

std::uint8_t Memory::getUninitMask(std::uint32_t address, std::size_t byteCount) const {
    std::uint8_t mask = 0;

    for (std::size_t i = 0; i < byteCount; ++i) {
        const std::uint32_t currentAddress = address + static_cast<std::uint32_t>(i);
        const Block* block = findBlock(blockIndexForAddress(currentAddress));
        const bool initialized =
            block != nullptr &&
            block->initialized.test(blockOffsetForAddress(currentAddress));

        if (!initialized) {
            // bit 0 соответствует первому байту чтения, bit 1 - следующему и так далее
            mask |= static_cast<std::uint8_t>(1u << i);
        }
    }

    return mask;
}

void Memory::notifyUninitRead(std::uint32_t address, std::size_t byteCount) const {
    if (!cellReadHandler_) [[likely]] {
        return;
    }

    const std::uint8_t mask = getUninitMask(address, byteCount);
    if (mask == 0) [[likely]] {
        return;
    }

    cellReadHandler_(address, byteCount, mask);
}

inline std::uint32_t Memory::readCellUnchecked(std::size_t cellIndex) const {
    const std::uint32_t blockIndex = static_cast<std::uint32_t>(cellIndex >> cellsPerBlockShift);
    const Block* block = findBlock(blockIndex);
    if (block == nullptr) [[unlikely]] {
        return 0;
    }

    return block->cells[cellIndex & (cellsPerBlock - 1)];
}

inline void Memory::writeCellUnchecked(std::size_t cellIndex, std::uint32_t value) {
    const std::uint32_t blockIndex = static_cast<std::uint32_t>(cellIndex >> cellsPerBlockShift);
    Block& block = getOrCreateBlock(blockIndex);
    block.cells[cellIndex & (cellsPerBlock - 1)] = value;
}

Memory::Block& Memory::getOrCreateBlock(std::uint32_t blockIndex) {
    if (cachedWriteBlock_ != nullptr && cachedWriteBlockIndex_ == blockIndex) [[likely]] {
        cachedReadBlockIndex_ = blockIndex;
        cachedReadBlock_ = cachedWriteBlock_;
        return *cachedWriteBlock_;
    }

    // блок создается только при записи или загрузке данных, а не при создании всей RAM
    Block& block = blocks_[blockIndex];
    cachedWriteBlockIndex_ = blockIndex;
    cachedWriteBlock_ = &block;

    // блок, найденный при записи, сразу можно использовать и для последующего чтения
    cachedReadBlockIndex_ = blockIndex;
    cachedReadBlock_ = &block;
    return block;
}

const Memory::Block* Memory::findBlock(std::uint32_t blockIndex) const {
    if (cachedReadBlock_ != nullptr && cachedReadBlockIndex_ == blockIndex) [[likely]] {
        return cachedReadBlock_;
    }

    const auto it = blocks_.find(blockIndex);
    if (it == blocks_.end()) {
        return nullptr;
    }

    cachedReadBlockIndex_ = blockIndex;
    cachedReadBlock_ = &it->second;
    return cachedReadBlock_;
}

void Memory::resetBlockCache() noexcept {
    cachedReadBlockIndex_ = 0;
    cachedReadBlock_ = nullptr;
    cachedWriteBlockIndex_ = 0;
    cachedWriteBlock_ = nullptr;
}

inline std::uint32_t Memory::blockIndexForAddress(std::uint32_t address) noexcept {
    return address >> blockShift;
}

inline std::size_t Memory::blockOffsetForAddress(std::uint32_t address) noexcept {
    return address & blockMask;
}

inline std::uint8_t Memory::readByteFromBlock(const Block& block, std::size_t blockOffset) noexcept {
    // blockOffset уже относится к конкретному блоку, поэтому здесь остается только выбрать ячейку и байт
    const std::size_t cellIndex = blockOffset >> cellShift;
    const std::size_t byteIndex = blockOffset & cellMask;
    const std::size_t shift = byteIndex * 8; // количество сдвигов вправо для получения нужного байта

    return static_cast<std::uint8_t>((block.cells[cellIndex] >> shift) & byteMask);
}

inline void Memory::writeByteToBlock(Block& block, std::size_t blockOffset, std::uint8_t value) noexcept {
    // запись байта не трогает остальные байты той же 4-байтовой ячейки
    const std::size_t cellIndex = blockOffset >> cellShift;
    const std::size_t byteIndex = blockOffset & cellMask;
    const std::size_t shift = byteIndex * 8;
    const std::uint32_t mask = byteMask << shift; // маска байта, который нужно заменить внутри 4-байтовой ячейки

    block.cells[cellIndex] = (block.cells[cellIndex] & ~mask) |
                             (static_cast<std::uint32_t>(value) << shift);
}

// получаю маску под конкретные байты std::uint32_t числа
inline std::uint32_t Memory::byteRangeMask(std::size_t byteOffset, std::size_t byteCount) noexcept {
    if (byteCount == cellSize) {
        return ~std::uint32_t{0};
    }

    return ((std::uint32_t{1} << (byteCount * 8)) - 1u) << (byteOffset * 8);
}

inline void Memory::clearBytesInCell(
    std::uint32_t& cell,
    std::size_t byteOffset,
    std::size_t byteCount
) noexcept {
    // очищаю логические байты ячейки через маску, не завися от endian хоста
    cell &= ~byteRangeMask(byteOffset, byteCount);
}

void Memory::clearBytesInBlock(Block& block, std::size_t blockOffset, std::size_t byteCount) noexcept {
    // диапазон уже лежит внутри одного блока, поэтому можно работать без поиска в unordered_map
    if (blockOffset == 0 && byteCount == blockSize) {
        block.cells.fill(0);
        return;
    }

    std::size_t cleared = 0;
    // очищаю в пределах одной ячейки
    if ((blockOffset & cellMask) != 0) {
        const std::size_t cellOffset = blockOffset & cellMask;
        const std::size_t bytesInCell = std::min(byteCount, cellSize - cellOffset);
        clearBytesInCell(block.cells[blockOffset >> cellShift], cellOffset, bytesInCell);
        cleared += bytesInCell;
    }

    const std::size_t firstFullCell = (blockOffset + cleared) >> cellShift;
    const std::size_t fullCellCount = (byteCount - cleared) >> cellShift;
    if (fullCellCount != 0) {
        // большие середины диапазона чистятся сразу по 32-битным ячейкам, а не по байтам
        std::fill_n(block.cells.begin() + static_cast<std::ptrdiff_t>(firstFullCell), fullCellCount, 0);
        cleared += fullCellCount * cellSize;
    }
    // очищаю в последней ячейке
    if (cleared < byteCount) {
        const std::size_t currentOffset = blockOffset + cleared;
        clearBytesInCell(block.cells[currentOffset >> cellShift], 0, byteCount - cleared);
    }
}

void Memory::markInitializedInBlock(Block& block, std::size_t blockOffset, std::size_t byteCount) {
    // сюда передается диапазон, который точно лежит внутри одного блока
    for (std::size_t i = 0; i < byteCount; ++i) {
        block.initialized.set(blockOffset + i);
    }
}

void Memory::resetInitializedInBlock(Block& block, std::size_t blockOffset, std::size_t byteCount) {
    // полный блок сбрасывается одним вызовом bitset::reset, частичные края остаются побайтовыми
    if (blockOffset == 0 && byteCount == blockSize) {
        block.initialized.reset();
        return;
    }

    for (std::size_t i = 0; i < byteCount; ++i) {
        block.initialized.reset(blockOffset + i);
    }
}

}
