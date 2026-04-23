add_executable(emulator
    src/main.cpp
    src/emulator.cpp
)

target_include_directories(emulator 
    PUBLIC include
)

target_link_libraries(emulator 
    PRIVATE common
)