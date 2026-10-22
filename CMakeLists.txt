cmake_minimum_required(VERSION 3.16)
project(chess_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

# Bundle the MinGW runtime into the .exe so it runs outside the MSYS2 shell
# (e.g. double-clicked, or launched by ChessX / Cute Chess).
if(MINGW)
    add_link_options(-static)
endif()

# Core engine: board, move generation, evaluation, search.
# No UI code lives here so it can later be built for a microcontroller.
add_library(engine STATIC
    engine/board.cpp
    engine/movegen.cpp
    engine/eval.cpp
    engine/search.cpp
)
target_include_directories(engine PUBLIC engine)

# Human vs AI in the terminal
add_executable(chess_cli play/cli/main.cpp)
target_link_libraries(chess_cli PRIVATE engine)
