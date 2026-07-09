#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "command_line.h"
#include "emulator.h"

namespace {

void printUsage(std::ostream& out) {
    out << "usage: emulator [--trace=disasm,ram-wr] [--trace-ticks=a,b-c] "
           "[--ram-size=<size>] [--warn=uninit-ram] "
           "[--trace-ram-addrs=a,b-c] <input.o>\n";
}

}

int main(int argc, char* argv[]) {
    emulator::CommandLineOptions options;
    try {
        options = emulator::parseCommandLine(argc, argv);
    }
    catch (const std::invalid_argument& error) {
        std::cerr << error.what() << '\n';
        printUsage(std::cerr);
        return 1;
    }

    try {
        emulator::Emulator emulator(options.ramSize);
        emulator.loadProgramFromFile(options.inputPath);

#if MCST_TRACING
        if (options.trace.disasm || options.trace.ramWrites) {
            emulator.setTraceTicks(std::move(options.trace.tickRanges));
        }
        if (options.trace.disasm) {
            emulator.enableDisasmTrace(std::cout);
        }
        if (options.trace.ramWrites) {
            emulator.enableRamWriteTrace(std::cout, std::move(options.trace.ramAddressRanges));
        }
#endif

        emulator.run();

        if (!options.trace.disasm && !options.trace.ramWrites) {
            std::cout << emulator.dumpRegisters() << '\n';
        }
        return 0;
    } 
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
