#pragma once
#include "board.h"

struct Move {
    Pos from, to;
    int score;
};

Move minimax(ChessBoard& board, int depth, bool minimize);

// Picks the engine's move with minimax and plays it on the board
Move AIMove(ChessBoard& board);
