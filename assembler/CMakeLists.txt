add_executable(assembler
    src/main.cpp
    src/assembler.cpp
)

target_include_directories(assembler PUBLIC include)
target_link_libraries(assembler PRIVATE common)