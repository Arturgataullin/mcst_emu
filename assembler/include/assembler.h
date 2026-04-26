#pragma once

#include <string>

namespace assembler {

class Assembler {
public:
    void assembleFile(const std::string& inputPath, const std::string& outputPath) const;
};

}