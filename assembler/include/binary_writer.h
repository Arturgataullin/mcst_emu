#pragma once 

#include <vector>
#include <cstdint>
#include <string>

namespace assembler {

class BinaryWriter {
public:
    static void write(const std::vector<std::uint8_t>& bytes, const std::string& path); 
};

}