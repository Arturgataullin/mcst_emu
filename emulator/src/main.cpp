#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "command_line.h"
#include "emulator.h"

namespace {

void printUsage(std::ostream& out) {
    out << "usage: emulator [--trace=disasm,ram-rw] [--trace-ticks=a,b-c] [--ram-size=<size>] \\
    [--warn=uninit-ram] [--trace-ramaddrs=a,b-c] <input.o>\n";
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
