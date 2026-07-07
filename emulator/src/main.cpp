#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "emulator.h"

namespace {

const std::string trace_str = "--trace=";
const std::string trace_ticks_str = "--trace-ticks=";

// constexpr std::string_view trace_str = "--trace=";
// constexpr std::string_view trace_ticks_str = "--trace-ticks=";

struct CommandLineOptions {
    std::string inputPath;
    bool traceDisasm = false;
#if MCST_TRACING
    std::vector<emulator::TickRange> traceRanges;
#endif
};

void printUsage(std::ostream& out) {
    out << "usage: emulator [--trace=disasm] [--trace-ticks=a,b-c] <input.o>\n";
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

int main(int argc, char* argv[]) {
    CommandLineOptions options;
    try {
        options = parseCommandLine(argc, argv);
    }
    catch (const std::invalid_argument& error) {
        std::cerr << error.what() << '\n';
        printUsage(std::cerr);
        return 1;
    }

    try {
        emulator::Emulator emulator;
        emulator.loadProgramFromFile(options.inputPath);

#if MCST_TRACING
        if (options.traceDisasm) {
            emulator.setTraceOutput(std::cout);
            emulator.setTraceRanges(std::move(options.traceRanges));
        }
#endif

        emulator.run();

        if (!options.traceDisasm) {
            std::cout << emulator.dumpRegisters() << '\n';
        }
        return 0;
    } 
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
