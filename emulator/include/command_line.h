#pragma once

#include "tick_ranges.h"
#include <string>

namespace emulator {

struct TraceOptions {
    bool disasm = false;
    bool ramWrites = false;
    std::vector<TickRange> tickRanges;
};

struct CommandLineOptions {
    std::string inputPath;
    std::size_t ramSize;
    TraceOptions trace;
};

CommandLineOptions parseCommandLine(int argc, char* argv[]);

}