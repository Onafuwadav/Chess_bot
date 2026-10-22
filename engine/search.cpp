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
        for (auto& to : possibleMoves(board, from.first, board.LastMoveTo, board.LastMoveFrom, board.LastMove[board.LastMoveTo])) {
            ChessBoard branch = board;
            branch.makeMove(from.first, to);
            Move option = minimax(branch, depth - 1, !minimize);
            if ((option.score > best_move.score && !minimize) || (option.score < best_move.score && minimize)) {
                best_move.score = option.score;
                best_move.from = from.first;
                best_move.to = to;
            }
        }
    }
    return best_move;
}

Move AIMove(ChessBoard& board) {
    bool minimize = board.turn == ChessBoard::Turn::white ? true : false;
    Move m = minimax(board, 4, minimize);
    board.makeMove(m.from, m.to);
    return m;
}
