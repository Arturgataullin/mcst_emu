#include "binary_writer.h"

#include <fstream>
#include <stdexcept>

namespace assembler {

void BinaryWriter::write(const std::vector<std::uint8_t>& bytes, const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("failed to open output file: " + path);
    }

    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));

    if (!out) {
        throw std::runtime_error("failed to write output file: " + path);
    }
}

}