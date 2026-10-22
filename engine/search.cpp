#include "search.h"
#include "eval.h"
#include "movegen.h"
#include <algorithm>

uint64_t searchNodes = 0;

namespace {

struct OrderedMove {
    Pos from, to;
    ChessBoard::Piece promotion;
    int order; // higher is searched first
};

// All legal moves, best-looking first, so alpha-beta finds good moves early and cuts more.
// Captures use MVV-LVA (most valuable victim, least valuable attacker): 10 * victim - attacker,
// so pawn takes queen (8900) comes before queen takes pawn (100). Promotions add the new piece's value.
std::vector<OrderedMove> orderedMoves(ChessBoard& board, bool capturesOnly = false) {
    std::vector<OrderedMove> moves;
    for (auto& from : board.moverPieces()) {
        int attacker = ChessBoard::pieceValues[from.second];
        bool pawn = from.second == ChessBoard::Piece::white_pawn || from.second == ChessBoard::Piece::black_pawn;
        for (auto& to : possibleMoves(board, from.first, board.LastMoveTo, board.LastMoveFrom, board.LastMovePiece, capturesOnly)) {
            int order = 0;
            auto victim = board.opponentPieces().find(to);
            if (victim != board.opponentPieces().end())
                order = 10000 + 10 * ChessBoard::pieceValues[victim->second] - attacker;
            else if (pawn && to.x != from.first.x)
                order = 10000 + 10 * 100 - attacker; // en passant takes a pawn

            // A pawn reaching the last rank is four moves, one per promotion piece
            bool promotion = isPromotion(board, from.first, to);
            for (ChessBoard::Piece piece : promotionPieces) {
                moves.push_back({ from.first, to, piece, promotion ? order + ChessBoard::pieceValues[piece] : order });
                if (!promotion)
                    break;
            }
        }
    }
    std::stable_sort(moves.begin(), moves.end(),
        [](const OrderedMove& a, const OrderedMove& b) { return a.order > b.order; });
    return moves;
}

// Quiescence search: at the end of the main search, keep playing captures (and promotions)
// until the position is quiet, so a capture on the last move isn't scored before the
// recapture (the "horizon effect").
// Stand pat: the side to move doesn't have to capture, so the static score is a floor
// for it, and if that already beats the window the opponent won't allow this position.
int quiesce(ChessBoard& board, int alpha, int beta, bool minimize) {
    ++searchNodes;
    int best = score(board);
    if (!minimize) {
        if (best >= beta) return best;
        alpha = std::max(alpha, best);
    } else {
        if (best <= alpha) return best;
        beta = std::min(beta, best);
    }

    for (const OrderedMove& move : orderedMoves(board, true)) {
        ChessBoard branch = board;
        branch.applyMove(move.from, move.to, move.promotion);
        int s = quiesce(branch, alpha, beta, !minimize);
        if (!minimize) {
            best = std::max(best, s);
            alpha = std::max(alpha, s);
        } else {
            best = std::min(best, s);
            beta = std::min(beta, s);
        }
        if (alpha >= beta)
            break;
    }
    return best;
}

} // namespace

// Alpha-beta: alpha is the score white (the maximiser) is already sure of, beta the score
// black (the minimiser) is already sure of. Once alpha >= beta the side choosing at the
// parent would never let the game reach this position, so the remaining moves are skipped.
Move minimax(ChessBoard& board, int depth, int alpha, int beta, bool minimize) {
    Move best_move;
    best_move.score = -1000000 + 2000000 * minimize;
    if (0 == depth) {
        best_move.score = quiesce(board, alpha, beta, minimize);
        return best_move;
    }
    ++searchNodes;
    // A drawn position is worth 0 to both sides, however much material is left
    if (board.drawReason()) {
        best_move.score = 0;
        return best_move;
    }

    for (const OrderedMove& move : orderedMoves(board)) {
        ChessBoard branch = board;
        branch.applyMove(move.from, move.to, move.promotion);
        Move option = minimax(branch, depth - 1, alpha, beta, !minimize);
        // (the first move is always taken, so a move is returned even if every move gets mated)
        if (best_move.from.x == -1 || (option.score > best_move.score && !minimize) || (option.score < best_move.score && minimize)) {
            best_move.score = option.score;
            best_move.from = move.from;
            best_move.to = move.to;
            best_move.promotion = move.promotion;
        }
        if (minimize)
            beta = std::min(beta, best_move.score);
        else
            alpha = std::max(alpha, best_move.score);
        if (alpha >= beta)
            return best_move; // cut-off
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
    Move m = minimax(board, 4, -INFINITE_SCORE, INFINITE_SCORE, minimize);
    board.makeMove(m.from, m.to, m.promotion);
    return m;
}
