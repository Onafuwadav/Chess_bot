#pragma once
#include "board.h"
#include <vector>

// Function to generate possible moves for a given piece
std::vector<Pos> possibleMoves(ChessBoard& board, const Pos& from, const Pos& LastTo, const Pos& Lastfrom, ChessBoard::Piece lastpiece);

// The pieces a pawn can promote to, most useful first
constexpr ChessBoard::Piece promotionPieces[4] = {
    ChessBoard::Piece::queen, ChessBoard::Piece::knight, ChessBoard::Piece::rook, ChessBoard::Piece::bishop };

// True if moving from -> to is a pawn reaching the last rank (so it is really four moves, one per promotion piece)
bool isPromotion(const ChessBoard& board, const Pos& from, const Pos& to);

// Letter used for a promotion piece in move text such as "e7e8q"
char promotionLetter(ChessBoard::Piece piece);
