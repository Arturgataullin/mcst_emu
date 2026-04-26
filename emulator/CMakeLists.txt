add_library(emulator_lib
    src/emulator.cpp
    src/opMemory.cpp
)

target_include_directories(emulator_lib
    PUBLIC include
)

target_link_libraries(emulator_lib
    PUBLIC common
)

add_executable(emulator
    src/main.cpp
)

target_link_libraries(emulator
    PRIVATE emulator_lib
)