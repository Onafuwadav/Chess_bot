#include "eval.h"

std::map<ChessBoard::Piece, int> ChessBoard::pieceValues{ {ChessBoard::Piece::king, 10000},
{ChessBoard::Piece::queen, 9}, {ChessBoard::Piece::black_pawn, 1}, {ChessBoard::Piece::white_pawn, 1},
{ChessBoard::Piece::bishop, 3},{ChessBoard::Piece::knight, 3},{ChessBoard::Piece::rook, 5}, };

// make different score function for ai to add psf/sfe
int score(const ChessBoard& board) {
    int sumWhite = 0;
    for (auto& p : board.white_pieces)
        sumWhite += ChessBoard::pieceValues[p.second];
    int sumBlack = 0;
    for (auto& p : board.black_pieces)
        sumBlack += ChessBoard::pieceValues[p.second];
    return sumWhite - sumBlack;
}
