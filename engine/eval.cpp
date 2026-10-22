#include "eval.h"
#include <cstdint>

// Values in centipawns (1 pawn = 100). The king's value cancels out since both sides always have one.
std::map<ChessBoard::Piece, int> ChessBoard::pieceValues{ {ChessBoard::Piece::king, 20000},
{ChessBoard::Piece::queen, 900}, {ChessBoard::Piece::black_pawn, 100}, {ChessBoard::Piece::white_pawn, 100},
{ChessBoard::Piece::bishop, 330},{ChessBoard::Piece::knight, 320},{ChessBoard::Piece::rook, 500}, };

namespace {

// Piece-square tables: a bonus or penalty for a piece standing on each square, from
// Tomasz Michniewski's "Simplified Evaluation Function". They are written the way white
// sees the board: first row is rank 8, first column is the a-file.
// int8_t keeps them to 64 bytes each (they fit in a microcontroller's flash).

const int8_t pawnTable[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0 };

const int8_t knightTable[64] = {
   -50,-40,-30,-30,-30,-30,-40,-50,
   -40,-20,  0,  0,  0,  0,-20,-40,
   -30,  0, 10, 15, 15, 10,  0,-30,
   -30,  5, 15, 20, 20, 15,  5,-30,
   -30,  0, 15, 20, 20, 15,  0,-30,
   -30,  5, 10, 15, 15, 10,  5,-30,
   -40,-20,  0,  5,  5,  0,-20,-40,
   -50,-40,-30,-30,-30,-30,-40,-50 };

const int8_t bishopTable[64] = {
   -20,-10,-10,-10,-10,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5, 10, 10,  5,  0,-10,
   -10,  5,  5, 10, 10,  5,  5,-10,
   -10,  0, 10, 10, 10, 10,  0,-10,
   -10, 10, 10, 10, 10, 10, 10,-10,
   -10,  5,  0,  0,  0,  0,  5,-10,
   -20,-10,-10,-10,-10,-10,-10,-20 };

const int8_t rookTable[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0 };

const int8_t queenTable[64] = {
   -20,-10,-10, -5, -5,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5,  5,  5,  5,  0,-10,
    -5,  0,  5,  5,  5,  5,  0, -5,
     0,  0,  5,  5,  5,  5,  0, -5,
   -10,  5,  5,  5,  5,  5,  0,-10,
   -10,  0,  5,  0,  0,  0,  0,-10,
   -20,-10,-10, -5, -5,-10,-10,-20 };

// Middlegame: stay castled behind the pawns
const int8_t kingMiddleTable[64] = {
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -20,-30,-30,-40,-40,-30,-30,-20,
   -10,-20,-20,-20,-20,-20,-20,-10,
    20, 20,  0,  0,  0,  0, 20, 20,
    20, 30, 10,  0,  0, 10, 30, 20 };

// Endgame: come to the centre and help
const int8_t kingEndTable[64] = {
   -50,-40,-30,-20,-20,-30,-40,-50,
   -30,-20,-10,  0,  0,-10,-20,-30,
   -30,-10, 20, 30, 30, 20,-10,-30,
   -30,-10, 30, 40, 40, 30,-10,-30,
   -30,-10, 30, 40, 40, 30,-10,-30,
   -30,-10, 20, 30, 30, 20,-10,-30,
   -30,-30,  0,  0,  0,  0,-30,-30,
   -50,-30,-30,-30,-30,-30,-30,-50 };

// Table index for a piece on (x, y). Board x = 1 is the h-file, so the column is 8 - x.
// White reads rank 8 as row 0; black sees the board flipped, so its rank 1 is row 0.
int tableIndex(const Pos& p, bool white) {
    int row = white ? 8 - p.y : p.y - 1;
    return row * 8 + (8 - p.x);
}

const int8_t* tableFor(ChessBoard::Piece piece) {
    switch (piece) {
    case ChessBoard::Piece::white_pawn:
    case ChessBoard::Piece::black_pawn: return pawnTable;
    case ChessBoard::Piece::knight: return knightTable;
    case ChessBoard::Piece::bishop: return bishopTable;
    case ChessBoard::Piece::rook: return rookTable;
    case ChessBoard::Piece::queen: return queenTable;
    default: return nullptr; // king: blended separately
    }
}

// Game phase: 24 with all pieces on the board, falling towards 0 as pieces come off
// (knight/bishop 1, rook 2, queen 4)
int gamePhase(const ChessBoard& board) {
    int phase = 0;
    for (const auto* side : { &board.white_pieces, &board.black_pieces })
        for (const auto& p : *side) {
            if (p.second == ChessBoard::Piece::knight || p.second == ChessBoard::Piece::bishop) phase += 1;
            if (p.second == ChessBoard::Piece::rook) phase += 2;
            if (p.second == ChessBoard::Piece::queen) phase += 4;
        }
    return phase < 24 ? phase : 24;
}

} // namespace

// Material + piece-square tables, white minus black, in centipawns.
// The king's square bonus is tapered: (middle * phase + end * (24 - phase)) / 24,
// so it moves smoothly from "hide" to "centralise" as pieces are traded.
int score(const ChessBoard& board) {
    int phase = gamePhase(board);
    int total = 0;
    for (int white = 0; white < 2; ++white) {
        int sign = white ? 1 : -1;
        for (const auto& p : white ? board.white_pieces : board.black_pieces) {
            int index = tableIndex(p.first, white);
            int value = ChessBoard::pieceValues[p.second];
            if (const int8_t* table = tableFor(p.second))
                value += table[index];
            else
                value += (kingMiddleTable[index] * phase + kingEndTable[index] * (24 - phase)) / 24;
            total += sign * value;
        }
    }
    return total;
}
