cmake_minimum_required(VERSION 3.20)
project(cpu_project VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(project_warnings INTERFACE)
target_compile_options(project_warnings INTERFACE
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-Wall;-Wextra>
)

add_subdirectory(common)
add_subdirectory(assembler)
add_subdirectory(emulator)

if(BUILD_TESTING)
    include(CTest)
    add_subdirectory(tests)
endif()
