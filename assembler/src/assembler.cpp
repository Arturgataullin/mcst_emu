#include "assembler.h"

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

#include "binary_writer.h"
#include "encoder.h"
#include "lexer.h"
#include "parser.h"

namespace assembler {

namespace {

std::string readTextFile(const std::string& inputPath) {
    std::ifstream in(inputPath, std::ios::binary);
    if (!in) {
        throw std::runtime_error("failed to open input file: " + inputPath);
    }

    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

}

void Assembler::assembleFile(const std::string& inputPath, const std::string& outputPath) const {
	const std::string source = readTextFile(inputPath);

	Lexer lexer(source);
	auto tokens = lexer.tokenize();

	Parser parser(std::move(tokens));
	Program program = parser.parseProgram();

	Encoder encoder;
	const auto bytes = encoder.encode(program);

	BinaryWriter::write(bytes, outputPath);
}

}