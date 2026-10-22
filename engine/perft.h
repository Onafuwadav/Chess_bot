#pragma once
#include "board.h"
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

// Counts every position reachable in exactly 'depth' moves. Comparing these
// counts with published values is the standard test for move generation.
uint64_t perft(ChessBoard& board, int depth);

// perft split by first move, e.g. {"e2e4", 600}, to find which move is wrong
std::vector<std::pair<std::string, uint64_t>> perftDivide(ChessBoard& board, int depth);
