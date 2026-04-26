#include <exception>
#include <iostream>

#include "emulator.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "usage: emulator <input.o>\n";
        return 1;
    }

    try {
        emulator::Emulator emulator;
        emulator.loadProgramFromFile(argv[1]);
        emulator.run();

        std::cout << emulator.dumpRegisters() << '\n';
        return 0;
    } 
    catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}