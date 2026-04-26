cmake_minimum_required(VERSION 3.20)
project(cpu_project VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

add_subdirectory(common)
add_subdirectory(assembler)
add_subdirectory(emulator)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()