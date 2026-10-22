#pragma once
#include "board.h"

// Material plus piece-square tables, in centipawns: positive = good for white
int score(const ChessBoard& board);
