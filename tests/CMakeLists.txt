include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)

FetchContent_MakeAvailable(Catch2)

add_executable(assembler_lexer_tests
    assembler/lexer/lexer_tests.cpp
)

add_executable(assembler_parser_tests
    assembler/parser/parser_tests.cpp
)

add_executable(assembler_encoder_tests
    assembler/encoder/encoder_tests.cpp
)

add_executable(assembler_tests
    assembler/assembler_tests.cpp
)

add_executable(emulator_memory_tests
    emulator/memory/memory_tests.cpp
)

add_executable(emulator_tests
    emulator/emulator_tests.cpp
)

target_link_libraries(assembler_lexer_tests PRIVATE assembler_lib Catch2::Catch2WithMain)

target_link_libraries(assembler_parser_tests PRIVATE assembler_lib Catch2::Catch2WithMain)

target_link_libraries(assembler_encoder_tests PRIVATE assembler_lib Catch2::Catch2WithMain)

target_link_libraries(assembler_tests PRIVATE assembler_lib Catch2::Catch2WithMain)

target_link_libraries(emulator_memory_tests PRIVATE emulator_lib Catch2::Catch2WithMain)

target_link_libraries(emulator_tests PRIVATE emulator_lib Catch2::Catch2WithMain)

include(Catch)

catch_discover_tests(assembler_lexer_tests)
catch_discover_tests(assembler_parser_tests)
catch_discover_tests(assembler_encoder_tests)
catch_discover_tests(assembler_tests)
catch_discover_tests(emulator_memory_tests)
catch_discover_tests(emulator_tests)