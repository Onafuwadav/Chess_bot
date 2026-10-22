#include "search.h"
#include "eval.h"
#include "movegen.h"

Move minimax(ChessBoard& board, int depth, bool minimize) {
    Move best_move;
    best_move.score = -1000000 + 2000000 * minimize;
    if (0 == depth) {
        best_move.score = score(board);
        return best_move;
    }

    for (auto& from : board.moverPieces()) {
        for (auto& to : possibleMoves(board, from.first, board.LastMoveTo, board.LastMoveFrom, board.LastMovePiece)) {
            // A pawn reaching the last rank is four moves, one per promotion piece
            bool promotion = isPromotion(board, from.first, to);
            for (ChessBoard::Piece piece : promotionPieces) {
                ChessBoard branch = board;
                branch.applyMove(from.first, to, piece);
                Move option = minimax(branch, depth - 1, !minimize);
                if ((option.score > best_move.score && !minimize) || (option.score < best_move.score && minimize)) {
                    best_move.score = option.score;
                    best_move.from = from.first;
                    best_move.to = to;
                    best_move.promotion = piece;
                }
                if (!promotion)
                    break;
            }
        }
    }
    // No legal moves: checkmate keeps the worst score for the side to move,
    // but stalemate is a draw
    if (best_move.from.x == -1 && !board.isKingUnderAttack(board.turn))
        best_move.score = 0;
    return best_move;
}

Move AIMove(ChessBoard& board) {
    // score() is white minus black, so white maximises and black minimises
    bool minimize = board.turn == ChessBoard::Turn::black;
    Move m = minimax(board, 4, minimize);
    board.makeMove(m.from, m.to, m.promotion);
    return m;
}
