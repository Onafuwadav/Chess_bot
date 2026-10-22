#pragma once
#include "board.h"

// make different score function for ai to add psf/sfe
// Material balance: positive = good for white
int score(const ChessBoard& board);
