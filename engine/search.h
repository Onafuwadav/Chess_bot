#pragma once
#include "board.h"

struct Move {
    Pos from, to;
    int score;
    ChessBoard::Piece promotion = ChessBoard::Piece::queen; // only used when a pawn reaches the last rank
};

Move minimax(ChessBoard& board, int depth, bool minimize);

// Picks the engine's move with minimax and plays it on the board
Move AIMove(ChessBoard& board);
