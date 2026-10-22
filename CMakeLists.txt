cmake_minimum_required(VERSION 3.16)
project(chess_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Bundle the MinGW runtime into the .exe so it runs outside the MSYS2 shell
# (e.g. double-clicked, or launched by ChessX / Cute Chess).
if(MINGW)
    add_link_options(-static)
endif()

# Compiler warnings for our own targets (not for downloaded libraries)
function(chess_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()
endfunction()

# Core engine: board, move generation, evaluation, search.
# No UI code lives here so it can later be built for a microcontroller.
add_library(engine STATIC
    engine/board.cpp
    engine/movegen.cpp
    engine/eval.cpp
    engine/search.cpp
    engine/perft.cpp
)
target_include_directories(engine PUBLIC engine)
chess_warnings(engine)

# Human vs AI in the terminal
add_executable(chess_cli play/cli/main.cpp)
target_link_libraries(chess_cli PRIVATE engine)
chess_warnings(chess_cli)

# Perft tests (Catch2 is downloaded at configure time)
option(CHESS_BUILD_TESTS "Build the perft tests" ON)
if(CHESS_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(Catch2
        URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.7.1.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

    enable_testing()
    add_executable(perft_tests testing/perft/perft_tests.cpp)
    target_link_libraries(perft_tests PRIVATE engine Catch2::Catch2WithMain)
    chess_warnings(perft_tests)

    include(Catch)
    catch_discover_tests(perft_tests)
endif()
