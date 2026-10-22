#pragma once
#include "board.h"
#include <cstdint>
#include <functional>

// Number of positions the search has visited (reset it before a search to measure one)
extern uint64_t searchNodes;

struct Move {
    Pos from, to;
    int score;
    ChessBoard::Piece promotion = ChessBoard::Piece::queen; // only used when a pawn reaches the last rank
};

// Score for checkmate (minus the number of moves to reach it, so faster mates score higher)
constexpr int MATE_SCORE = 1000000;

// Larger than any real score, used as the starting alpha/beta window
constexpr int INFINITE_SCORE = 10000000;

// Minimax with alpha-beta pruning, from white's point of view (white maximises).
// Call with alpha = -INFINITE_SCORE, beta = INFINITE_SCORE; ply is the distance from the root.
Move minimax(ChessBoard& board, int depth, int alpha, int beta, bool minimize, int ply = 0);

// Iterative deepening up to maxDepth, stopping after timeLimitMs (0 = no time limit).
// onDepth, if given, is called after each finished depth (used for UCI "info" lines).
Move searchBestMove(ChessBoard& board, int maxDepth, int timeLimitMs,
    const std::function<void(int depth, const Move& best)>& onDepth = nullptr);

// Asks a running searchBestMove to finish now and return its best move so far (safe from another thread)
void stopSearch();

// Picks the engine's move and plays it on the board
Move AIMove(ChessBoard& board, int timeLimitMs = 2000);
