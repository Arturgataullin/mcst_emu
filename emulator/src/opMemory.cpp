#include "opMemory.h"

#include <limits>
#include <stdexcept>
#include <utility>

namespace emulator {

// снаружи память адресуется байтами, внутри каждый выделенный блок хранит массив 4-байтовых ячеек
constexpr std::size_t cellSize = 4;
constexpr std::size_t cellMask = 3;
constexpr std::size_t cellShift = 2;
constexpr std::size_t blockShift = 12;
constexpr std::size_t blockMask = Memory::blockSize - 1;
constexpr std::uint32_t byteMask = 0xffu;

static_assert((cellSize & (cellSize - 1)) == 0, "cellSize must be power of two");
static_assert(cellMask == cellSize - 1);
static_assert((std::size_t{1} << cellShift) == cellSize);
static_assert((Memory::blockSize & (Memory::blockSize - 1)) == 0, "blockSize must be power of two");
static_assert(blockMask == Memory::blockSize - 1);
static_assert((std::size_t{1} << blockShift) == Memory::blockSize);

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
}

// загружает bytes в память, начиная с baseAddress включительно
void Memory::load(const std::vector<std::uint8_t>& bytes, std::uint32_t baseAddress) {
    checkRange(baseAddress, bytes.size());

    for (std::size_t i = 0; i < bytes.size(); ++i) {
        // загрузка программы не является исполненной командой записи в RAM
        const std::uint32_t address = baseAddress + static_cast<std::uint32_t>(i);
        write8Unchecked(address, bytes[i]);
        markInitialized(address, 1);
    }
}

std::uint8_t Memory::read8Unchecked(std::uint32_t address) const {
    const Block* block = findBlock(blockIndexForAddress(address));
    if (block == nullptr) {
        // невыделенный блок читается как нули, но его байты остаются неинициализированными
        return 0;
    }

    const std::size_t blockOffset = blockOffsetForAddress(address);
    const std::size_t cellIndex = blockOffset >> cellShift; // заменяю / 4 на более быстрый >> 2
    const std::size_t byteIndex = blockOffset & cellMask;
    const std::size_t shift = byteIndex * 8; // количество сдвигов вправо для получения необходимого байта

    return static_cast<std::uint8_t>((block->cells[cellIndex] >> shift) & byteMask);
}

void Memory::write8Unchecked(std::uint32_t address, std::uint8_t value) {
    Block& block = getOrCreateBlock(blockIndexForAddress(address));
    const std::size_t blockOffset = blockOffsetForAddress(address);
    const std::size_t cellIndex = blockOffset >> cellShift;
    const std::size_t byteIndex = blockOffset & cellMask;
    const std::size_t shift = byteIndex * 8;
    const std::uint32_t mask = byteMask << shift; // с помощью этой маски обнуляю байт, который нужно заменить

    block.cells[cellIndex] = (block.cells[cellIndex] & ~mask) |
                             (static_cast<std::uint32_t>(value) << shift);
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
    if (!cellWriteHandler_) {
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
    const std::size_t lastCellIndex = (static_cast<std::size_t>(address) + 3) >> cellShift;
    const std::uint32_t firstOldData = readCellUnchecked(firstCellIndex);
    const std::uint32_t lastOldData = readCellUnchecked(lastCellIndex);

    writeUnchecked(address, value, 4);
    markInitialized(address, 4);
    notifyCellWrites(firstCellIndex, firstOldData, lastCellIndex, lastOldData);
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

void Memory::checkAddress(std::uint32_t address) const {
    checkRange(address, 1);
}

void Memory::checkRange(std::uint32_t address, std::size_t accessSize) const {
    // не считаем address + accessSize напрямую, чтобы не получить переполнение
    if (accessSize > size_ || static_cast<std::size_t>(address) > size_ - accessSize) [[unlikely]] {
        throw std::runtime_error("memory range access out of range");
    }
}

void Memory::markInitialized(std::uint32_t address, std::size_t byteCount) {
    // факт записи важнее значения: даже запись нуля делает байт инициализированным
    for (std::size_t i = 0; i < byteCount; ++i) {
        const std::uint32_t currentAddress = address + static_cast<std::uint32_t>(i);
        Block& block = getOrCreateBlock(blockIndexForAddress(currentAddress));
        block.initialized.set(blockOffsetForAddress(currentAddress));
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
    if (!cellReadHandler_) {
        return;
    }

    const std::uint8_t mask = getUninitMask(address, byteCount);
    if (mask == 0) {
        return;
    }

    cellReadHandler_(address, byteCount, mask);
}

std::uint32_t Memory::readCellUnchecked(std::size_t cellIndex) const {
    const std::uint32_t blockIndex = static_cast<std::uint32_t>(cellIndex / cellsPerBlock);
    const Block* block = findBlock(blockIndex);
    if (block == nullptr) {
        return 0;
    }

    return block->cells[cellIndex & (cellsPerBlock - 1)];
}

Memory::Block& Memory::getOrCreateBlock(std::uint32_t blockIndex) {
    // блок создается только при записи или загрузке данных, а не при создании всей RAM
    return blocks_[blockIndex];
}

const Memory::Block* Memory::findBlock(std::uint32_t blockIndex) const {
    const auto it = blocks_.find(blockIndex);
    if (it == blocks_.end()) {
        return nullptr;
    }
    return &it->second;
}

std::uint32_t Memory::blockIndexForAddress(std::uint32_t address) noexcept {
    return address >> blockShift;
}

std::size_t Memory::blockOffsetForAddress(std::uint32_t address) noexcept {
    return address & blockMask;
}

}
