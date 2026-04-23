#include <iostream>

#include "lexer.h"

using namespace assembler;

int main() {
    std::cout << "Sdsd\n";
    Lexer lx("\n\n\n");
    auto res = lx.tokenize();
    std::cout << res.size() << "\n";
    for (const auto& elem : res) {
        std::cout << toString(elem.type) << "\n";
    }
    return 0;
}