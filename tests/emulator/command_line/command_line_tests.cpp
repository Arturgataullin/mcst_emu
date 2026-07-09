#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "command_line.h"

#include <string>
#include <string_view>
#include <vector>

using Catch::Matchers::ContainsSubstring;

namespace {

emulator::CommandLineOptions parse(std::vector<std::string> arguments) {
    arguments.insert(arguments.begin(), "emulator");

    std::vector<char*> argv;
    argv.reserve(arguments.size());
    for (std::string& argument : arguments) {
        argv.push_back(argument.data());
    }

    return emulator::parseCommandLine(static_cast<int>(argv.size()), argv.data());
}

}

TEST_CASE("command line parser accepts input path with defaults") {
    const auto options = parse({"program.o"});

    REQUIRE(options.inputPath == "program.o");
    REQUIRE(options.ramSize == common::opMemorySize);
    REQUIRE_FALSE(options.trace.disasm);
    REQUIRE_FALSE(options.trace.ramWrites);
    REQUIRE_FALSE(options.warnings.uninitializedRam);
}

TEST_CASE("command line parser parses trace modes as a comma-separated list") {
    const auto options = parse({"--trace=disasm,ram-wr", "program.o"});

    REQUIRE(options.trace.disasm);
    REQUIRE(options.trace.ramWrites);
}

TEST_CASE("command line parser parses ram size") {
    const auto options = parse({"--ram-size=4096", "program.o"});

    REQUIRE(options.ramSize == 4096);
}

TEST_CASE("command line parser parses trace tick ranges") {
    const auto options = parse({"--trace=disasm", "--trace-ticks=7-10,0", "program.o"});

    REQUIRE(options.trace.tickRanges.size() == 2);
    REQUIRE(options.trace.tickRanges[0].begin == 7);
    REQUIRE(options.trace.tickRanges[0].end == 10);
    REQUIRE(options.trace.tickRanges[1].begin == 0);
    REQUIRE(options.trace.tickRanges[1].end == 0);
}

TEST_CASE("command line parser parses ram write address ranges") {
    const auto options = parse({"--trace=ram-wr", "--trace-ram-addrs=4-7,16", "program.o"});

    REQUIRE(options.trace.ramWrites);
    REQUIRE(options.trace.ramAddressRanges.size() == 2);
    REQUIRE(options.trace.ramAddressRanges[0].begin == 4);
    REQUIRE(options.trace.ramAddressRanges[0].end == 7);
    REQUIRE(options.trace.ramAddressRanges[1].begin == 16);
    REQUIRE(options.trace.ramAddressRanges[1].end == 16);
}

TEST_CASE("command line parser parses warning modes") {
    const auto options = parse({"--warn=uninit-ram", "program.o"});

    REQUIRE(options.warnings.uninitializedRam);
}

TEST_CASE("command line parser rejects unknown trace mode") {
    REQUIRE_THROWS_WITH(
        parse({"--trace=raw-wr", "program.o"}),
        ContainsSubstring("unsupported trace mode")
    );
}

TEST_CASE("command line parser rejects invalid ram size") {
    REQUIRE_THROWS_WITH(
        parse({"--ram-size=3", "program.o"}),
        ContainsSubstring("multiple of 4")
    );
}

TEST_CASE("command line parser rejects tick ranges without trace") {
    REQUIRE_THROWS_WITH(
        parse({"--trace-ticks=1-2", "program.o"}),
        ContainsSubstring("--trace-ticks requires --trace")
    );
}

TEST_CASE("command line parser rejects ram address ranges without ram write trace") {
    REQUIRE_THROWS_WITH(
        parse({"--trace=disasm", "--trace-ram-addrs=1-2", "program.o"}),
        ContainsSubstring("--trace-ram-addrs requires --trace=ram-wr")
    );
}

TEST_CASE("command line parser rejects duplicate option") {
    REQUIRE_THROWS_WITH(
        parse({"--trace=disasm", "--trace=ram-wr", "program.o"}),
        ContainsSubstring("--trace specified more than once")
    );
}
