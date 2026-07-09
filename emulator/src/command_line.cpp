#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "command_line.h"

namespace emulator {

namespace {

    enum class OptionName {
        Trace,
        TraceTicks,
        RamSize,
        TraceRamAddresses,
        Warn
    };

    constexpr std::string_view trace_str = "--trace=";
    constexpr std::string_view trace_ticks_str = "--trace-ticks=";
    constexpr std::string_view ram_size_str = " --ram-size=";
    constexpr std::string_view trace_ram_addresses_str = "--trace-ram-addrs=";
    constexpr std::string_view warn_str = "--warn=";

    enum class TraceMode {
        Disasm,
        RamWrite
    };

    constexpr std::string_view disasm_str = "disasm";
    constexpr std::string_view ram_write_str = "ram-wr";

    enum class WarningMode {
        UninitializedRam
    };

    constexpr std::string_view uninitialized_ram_str = "uninit-ram";
    

    struct CommandLineOptions {
        std::string inputPath;
        std::size_t ramSize;
        std::vector<TraceMode> traceModes;
        std::vector<TickRange> traceTicks;
        std::vector<AddressRange> traceRamAddresses;
        std::vector<WarningMode> warnings;
    };

}



CommandLineOptions parseCommandLine(int argc, char* argv[]) {
    CommandLineOptions options;
    bool traceTicksSeen = false;
#if MCST_TRACING
    std::string traceTicksText;
#endif

    for (int i = 1; i < argc; ++i) {
        const std::string_view argument = argv[i];

        if (argument == "--trace=disasm") {
            options.traceDisasm = true;
        }
        else if (argument.starts_with(trace_str)) {
            throw std::invalid_argument(
                "unsupported trace format: " + std::string(argument.substr(trace_str.size()))
            );
        }
        else if (argument.starts_with(trace_ticks_str)) {
            if (traceTicksSeen) {
                throw std::invalid_argument("--trace-ticks specified more than once");
            }
            traceTicksSeen = true;
#if MCST_TRACING
            // отбрасываем префикс "--trace-ticks=" и сохраняем только диапазоны
            const std::string_view value = argument.substr(trace_ticks_str.size());
            traceTicksText.assign(value.data(), value.size());
#endif
        }
        else if (argument.starts_with('-')) {
            throw std::invalid_argument("unknown option: " + std::string(argument));
        }
        else if (!options.inputPath.empty()) {
            throw std::invalid_argument("more than one input file specified");
        }
        else {
            options.inputPath.assign(argument.data(), argument.size());
        }
    }

    if (options.inputPath.empty()) {
        throw std::invalid_argument("input file is not specified");
    }
    if (traceTicksSeen && !options.traceDisasm) {
        throw std::invalid_argument("--trace-ticks requires --trace=disasm");
    }

#if MCST_TRACING
    if (traceTicksSeen) {
        options.traceRanges = emulator::parseTickRanges(traceTicksText);
    }
#else
    // опция недоступна, если поддержка трассировки исключена при сборке
    if (options.traceDisasm) {
        throw std::invalid_argument("tracing support is disabled at build time");
    }
#endif

    return options;
}

}