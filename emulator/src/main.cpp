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
    out << "usage: emulator [options] <input.o>\n";
}

void printHelp(std::ostream& out) {
    printUsage(out);
    out
        << '\n'
        << "functional processor emulator\n"
        << '\n'
        << "options:\n"
        << "  --help                         show this help and exit\n"
        << "  --ram-size=<size>              set RAM size in bytes; value must be a positive multiple of 4\n"
        << "  --warn=uninit-ram              warn when a load reads uninitialized RAM bytes\n"
        << '\n'
        << "trace options:\n";
#if MCST_TRACING
    out
        << "  --trace=disasm                 print instruction trace\n"
        << "  --trace=ram-wr                 print RAM cell write trace\n"
        << "  --trace=disasm,ram-wr          enable several trace modes at once\n"
        << "  --trace-ticks=a,b-c            trace only selected ticks; ranges are inclusive\n"
        << "  --trace-ram-addrs=a,b-c        print RAM write trace only for selected byte addresses\n";
#else
    out
        << "  tracing is disabled in this build; configure with -DTRACING=ON to enable trace options\n";
#endif
    out
        << '\n'
        << "examples:\n"
        << "  emulator program.o\n"
        << "  emulator --ram-size=4096 program.o\n"
        << "  emulator --warn=uninit-ram program.o\n";
#if MCST_TRACING
    out
        << "  emulator --trace=disasm program.o\n"
        << "  emulator --trace=ram-wr --trace-ram-addrs=64-127 program.o\n"
        << "  emulator --trace=disasm,ram-wr --trace-ticks=0-10,25 program.o\n";
#endif
    out
        << '\n'
        << "notes:\n"
        << "  input.o must be a binary program whose size is a multiple of 4 bytes\n"
        << "  memory loads use little-endian byte order as required by the ISA\n";
}

}

int main(int argc, char* argv[]) {
    emulator::CommandLineOptions options;
    try {
        options = emulator::parseCommandLine(argc, argv);
        if (options.isNeedHelp) {
            printHelp(std::cout);
            return 0;
        }
    }
    catch (const std::invalid_argument& error) {
        std::cerr << error.what() << '\n';
        printUsage(std::cerr);
        return 1;
    }

    try {
        emulator::Emulator emulator(options.ramSize);
        emulator.loadProgramFromFile(options.inputPath);

        if (options.warnings.uninitializedRam) {
            emulator.enableUninitializedRamWarnings(std::cerr);
        }

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
