#include <exception>
#include <iostream>

#include "assembler.h"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "usage: assembler <input.s> <output.o>\n";
        return 1;
    }

    try {
        assembler::Assembler assembler;
        assembler.assembleFile(argv[1], argv[2]);
        return 0;
    } 
    catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}