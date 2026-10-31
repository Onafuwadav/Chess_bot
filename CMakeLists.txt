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

# UCI engine for chess GUIs (ChessX, Cute Chess) and cutechess-cli matches
find_package(Threads REQUIRED)
add_executable(chess_uci play/uci/main.cpp)
target_link_libraries(chess_uci PRIVATE engine Threads::Threads)
chess_warnings(chess_uci)

# Keep every engine version: after each build whose source changed, copy chess_uci.exe to
# old_builds/chess_uci_vN.exe and log it in old_builds/versions.txt. Off by default so CI and
# other clones don't archive; turn on once with: cmake -B build -DCHESS_ARCHIVE_BUILDS=ON
option(CHESS_ARCHIVE_BUILDS "Archive each new chess_uci.exe to old_builds/" OFF)
set(CHESS_ARCHIVE_DIR "${CMAKE_SOURCE_DIR}/old_builds" CACHE PATH "Where archived engine builds go")
if(CHESS_ARCHIVE_BUILDS)
    # git records which commit each version came from (MSYS2's shell may not have it on PATH)
    find_program(CHESS_GIT git HINTS "$ENV{ProgramFiles}/Git/cmd" "C:/Program Files/Git/cmd")
    add_custom_command(TARGET chess_uci POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -DEXE=$<TARGET_FILE:chess_uci>
            -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
            -DARCHIVE_DIR=${CHESS_ARCHIVE_DIR}
            -DGIT=${CHESS_GIT}
            -P ${CMAKE_SOURCE_DIR}/cmake/archive_build.cmake
        VERBATIM)
endif()

# Search benchmark: positions searched and time at a fixed depth
add_executable(engine_bench testing/bench/bench.cpp)
target_link_libraries(engine_bench PRIVATE engine)
chess_warnings(engine_bench)

# Tests: perft (move generation) and search (Catch2 is downloaded at configure time)
option(CHESS_BUILD_TESTS "Build the engine tests" ON)
if(CHESS_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(Catch2
        URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.7.1.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

    enable_testing()
    add_executable(engine_tests
        testing/perft/perft_tests.cpp
        testing/search/search_tests.cpp
        testing/search/eval_tests.cpp
        testing/rules/draw_tests.cpp
    )
    target_link_libraries(engine_tests PRIVATE engine Catch2::Catch2WithMain)
    chess_warnings(engine_tests)

    include(Catch)
    catch_discover_tests(engine_tests)
endif()
