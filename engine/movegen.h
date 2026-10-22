#pragma once
#include "board.h"
#include <vector>

// Function to generate possible moves for a given piece
std::vector<Pos> possibleMoves(ChessBoard& board, const Pos& from, const Pos& LastTo, const Pos& Lastfrom, ChessBoard::Piece lastpiece);
