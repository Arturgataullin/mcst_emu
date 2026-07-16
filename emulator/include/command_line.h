#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "isa.h"
#include "ranges.h"

#ifndef MCST_TRACING
#define MCST_TRACING 1
#endif

namespace emulator {

// флаги здесь удобнее enum-вектора: main и Emulator сразу видят, что именно включено
struct TraceOptions {
    bool disasm = false;
    bool ramWrites = false;
    std::vector<TickRange> tickRanges;
    std::vector<AddressRange> ramAddressRanges;
};

// отдельная структура под предупреждения, чтобы не смешивать их с трассировкой
struct WarningOptions {
    bool uninitializedRam = false;
};

struct CommandLineOptions {
    std::string inputPath;
    std::size_t ramSize = common::opMemorySize;
    TraceOptions trace;
    WarningOptions warnings;
    bool isNeedHelp = false;
    bool clearStack = false;
};

[[nodiscard]] CommandLineOptions parseCommandLine(int argc, char* argv[]);

}
