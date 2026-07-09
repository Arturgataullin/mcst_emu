add_library(emulator_lib
    src/command_line.cpp
    src/emulator.cpp
    src/opMemory.cpp
    src/ranges.cpp
)

if(TRACING)
    target_sources(emulator_lib PRIVATE src/trace.cpp)
endif()

target_include_directories(emulator_lib
    PUBLIC include
)

target_link_libraries(emulator_lib
    PUBLIC common
)

target_compile_definitions(emulator_lib
    PUBLIC MCST_TRACING=$<BOOL:${TRACING}>
)

add_executable(emulator
    src/main.cpp
)

target_link_libraries(emulator
    PRIVATE emulator_lib
)
