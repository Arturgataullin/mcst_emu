#include "command_line.h"

#include <array>
#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace emulator {
namespace {

enum class OptionName {
    Trace,
    TraceTicks,
    RamSize,
    TraceRamAddresses,
    Warn
};

struct OptionSpec {
    std::string_view prefix;
    OptionName name;
    std::string_view displayName;
};

struct ParsedOption {
    OptionName name;
    std::string_view value;
    std::string_view displayName;
};

// чтобы добавить новую опцию, достаточно добавить ее сюда и обработчик в switch ниже
constexpr std::array optionSpecs = {
    OptionSpec{"--trace=", OptionName::Trace, "--trace"},
    OptionSpec{"--trace-ticks=", OptionName::TraceTicks, "--trace-ticks"},
    OptionSpec{"--ram-size=", OptionName::RamSize, "--ram-size"},
    OptionSpec{"--trace-ram-addrs=", OptionName::TraceRamAddresses, "--trace-ram-addrs"},
    OptionSpec{"--warn=", OptionName::Warn, "--warn"},
};

constexpr std::size_t optionIndex(OptionName name) noexcept {
    return static_cast<std::size_t>(name);
}

constexpr std::size_t optionCount = optionIndex(OptionName::Warn) + 1;

// определяет только имя опции и отделяет значение после '=', само значение парсится отдельно
std::optional<ParsedOption> parseOption(std::string_view argument) {
    for (const OptionSpec& spec : optionSpecs) {
        if (argument.starts_with(spec.prefix)) {
            return ParsedOption{
                spec.name,
                argument.substr(spec.prefix.size()),
                spec.displayName
            };
        }
    }

    return std::nullopt;
}

// общая логика для опций вида --trace=a,b и --warn=a,b
template <typename Handler>
void parseCommaSeparatedList(
    std::string_view text,
    std::string_view optionName,
    Handler handleValue
) {
    if (text.empty()) {
        throw std::invalid_argument(std::string(optionName) + " requires a non-empty value");
    }

    std::size_t position = 0;
    while (position <= text.size()) {
        // получаю очередное значение из последовательности
        const std::size_t comma = text.find(',', position);
        const std::size_t tokenEnd = comma == std::string_view::npos ? text.size() : comma;
        const std::string_view token = text.substr(position, tokenEnd - position);

        if (token.empty()) {
            throw std::invalid_argument("empty value in " + std::string(optionName));
        }

        handleValue(token);

        if (comma == std::string_view::npos) {
            break;
        }
        position = comma + 1;
    }
}

std::uint64_t parseUnsigned(std::string_view text, std::string_view optionName) {
    if (text.empty()) {
        throw std::invalid_argument(std::string(optionName) + " requires a non-empty value");
    }

    std::uint64_t value = 0;
    const char* first = text.data();
    const char* last = first + text.size();
    const auto result = std::from_chars(first, last, value);

    if (result.ec != std::errc{} || result.ptr != last) {
        throw std::invalid_argument(
            "invalid value for " + std::string(optionName) + ": " + std::string(text)
        );
    }

    return value;
}

std::size_t parseRamSize(std::string_view text) {
    const std::uint64_t value = parseUnsigned(text, "--ram-size");
    // адреса внутри эмулируемой машины 32-битные, значит максимум адресуемо 4 GiB
    constexpr std::uint64_t maxGuestMemorySize = std::uint64_t{1} << 32;

    if (value == 0) {
        throw std::invalid_argument("--ram-size must be positive");
    }

    if (value % 4 != 0) {
        throw std::invalid_argument("--ram-size must be a multiple of 4 bytes");
    }

    if (value > maxGuestMemorySize) {
        throw std::invalid_argument("--ram-size exceeds 32-bit address space");
    }

    if (value > std::numeric_limits<std::size_t>::max()) {
        throw std::invalid_argument("--ram-size is too large for this platform");
    }

    return static_cast<std::size_t>(value);
}

void setTraceMode(TraceOptions& trace, std::string_view value) {
    if (value == "disasm") {
        if (trace.disasm) {
            throw std::invalid_argument("duplicate trace mode: disasm");
        }
        trace.disasm = true;
        return;
    }

    if (value == "ram-wr") {
        if (trace.ramWrites) {
            throw std::invalid_argument("duplicate trace mode: ram-wr");
        }
        trace.ramWrites = true;
        return;
    }

    throw std::invalid_argument("unsupported trace mode: " + std::string(value));
}

void parseTraceModes(TraceOptions& trace, std::string_view text) {
    parseCommaSeparatedList(text, "--trace", [&trace](std::string_view value) {
        setTraceMode(trace, value);
    });
}

void setWarningMode(WarningOptions& warnings, std::string_view value) {
    if (value == "uninit-ram") {
        if (warnings.uninitializedRam) {
            throw std::invalid_argument("duplicate warning mode: uninit-ram");
        }
        warnings.uninitializedRam = true;
        return;
    }

    throw std::invalid_argument("unsupported warning mode: " + std::string(value));
}

void parseWarnings(WarningOptions& warnings, std::string_view text) {
    parseCommaSeparatedList(text, "--warn", [&warnings](std::string_view value) {
        setWarningMode(warnings, value);
    });
}

bool anyTraceEnabled(const TraceOptions& trace) noexcept {
    return trace.disasm || trace.ramWrites;
}

void validateOptions(
    const CommandLineOptions& options,
    bool traceTicksSeen,
    bool traceRamAddressesSeen
) {
    if (options.inputPath.empty()) {
        throw std::invalid_argument("input file is not specified");
    }

    // здесь проверяются связи между опциями, которые нельзя проверить при разборе одной опции
#if MCST_TRACING
    if (traceTicksSeen && !anyTraceEnabled(options.trace)) {
        throw std::invalid_argument("--trace-ticks requires --trace");
    }

    if (traceRamAddressesSeen && !options.trace.ramWrites) {
        throw std::invalid_argument("--trace-ram-addrs requires --trace=ram-wr");
    }
#else
    if (anyTraceEnabled(options.trace) || traceTicksSeen || traceRamAddressesSeen) {
        throw std::invalid_argument("tracing support is disabled at build time");
    }
#endif
}

}

CommandLineOptions parseCommandLine(int argc, char* argv[]) {
    CommandLineOptions options;
    std::array<bool, optionCount> seenOptions{};
    bool traceTicksSeen = false;
    bool traceRamAddressesSeen = false;

    for (int i = 1; i < argc; ++i) {
        const std::string_view argument = argv[i];

        if (const auto parsed = parseOption(argument)) {
            const std::size_t index = optionIndex(parsed->name);
            if (seenOptions[index]) {
                throw std::invalid_argument(
                    std::string(parsed->displayName) + " specified more than once"
                );
            }
            seenOptions[index] = true;

            // после определения имени опции дальше работает только парсер ее значения
            switch (parsed->name) {
                case OptionName::Trace:
                    parseTraceModes(options.trace, parsed->value);
                    break;
                case OptionName::TraceTicks:
                    traceTicksSeen = true;
                    options.trace.tickRanges = parseTickRanges(parsed->value);
                    break;
                case OptionName::RamSize:
                    options.ramSize = parseRamSize(parsed->value);
                    break;
                case OptionName::TraceRamAddresses:
                    traceRamAddressesSeen = true;
                    options.trace.ramAddressRanges = parseAddressRanges(parsed->value);
                    break;
                case OptionName::Warn:
                    parseWarnings(options.warnings, parsed->value);
                    break;
            }
        }
        else if (argument.starts_with('-')) {
            throw std::invalid_argument("unknown option: " + std::string(argument));
        }
        else if (!options.inputPath.empty()) {
            throw std::invalid_argument("more than one input file specified");
        }
        else {
            options.inputPath.assign(argument.data(), argument.size());
        }
    }

    validateOptions(options, traceTicksSeen, traceRamAddressesSeen);
    return options;
}

}
