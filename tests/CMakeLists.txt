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

target_link_libraries(assembler_lexer_tests PRIVATE assembler_lib Catch2::Catch2WithMain)
target_link_libraries(assembler_parser_tests PRIVATE assembler_lib Catch2::Catch2WithMain)
target_link_libraries(assembler_encoder_tests PRIVATE assembler_lib Catch2::Catch2WithMain)
target_link_libraries(assembler_tests PRIVATE assembler_lib Catch2::Catch2WithMain)