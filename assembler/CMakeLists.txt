add_library(assembler_lib
    src/assembler.cpp
    src/lexer.cpp
    src/parser.cpp
    src/token.cpp
)

target_link_libraries(assembler_lib 
    PUBLIC common
)

target_include_directories(assembler_lib 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(assembler
    src/main.cpp
)

target_link_libraries(assembler
    PRIVATE assembler_lib
)

target_include_directories(assembler 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
)
