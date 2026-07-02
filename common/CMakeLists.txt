add_library(common
    src/isa.cpp
)

target_include_directories(common 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(common
    PUBLIC project_warnings
)
