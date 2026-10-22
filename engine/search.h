#pragma once
#include "board.h"
#include <cstdint>

// Number of positions the search has visited (reset it before a search to measure one)
extern uint64_t searchNodes;

struct Move {
    Pos from, to;
    int score;
    ChessBoard::Piece promotion = ChessBoard::Piece::queen; // only used when a pawn reaches the last rank
};

Move minimax(ChessBoard& board, int depth, bool minimize);

// Picks the engine's move with minimax and plays it on the board
Move AIMove(ChessBoard& board);
