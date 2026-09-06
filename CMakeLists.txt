cmake_minimum_required(VERSION 3.16)

project(chash VERSION 0.1.0 LANGUAGES CXX)

add_library(chash_core
    builtins.cpp
    executor.cpp
    history.cpp
    interpreter.cpp
    job_table.cpp
    parser.cpp
    prompt.cpp
    signals.cpp
    tokenizer.cpp
)

target_include_directories(chash_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(chash_core PUBLIC cxx_std_17)
set_target_properties(chash_core PROPERTIES
    CXX_EXTENSIONS OFF
)

target_compile_options(chash_core PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_executable(chash main.cpp)
target_link_libraries(chash PRIVATE chash_core)
target_compile_options(chash PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
