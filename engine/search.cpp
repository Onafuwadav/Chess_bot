#include "search.h"
#include "eval.h"
#include "movegen.h"
#include <algorithm>
#include <atomic>
#include <chrono>

uint64_t searchNodes = 0;

namespace {

// Time control: the search stops once the deadline passes (checked every 1024 positions)
// or when stopSearch() is called (atomic, since UCI calls it from another thread)
bool timeLimited = false;
std::atomic<bool> stopped{ false };
std::chrono::steady_clock::time_point deadline;

bool outOfTime() {
    if (timeLimited && !stopped && (searchNodes & 1023) == 0 && std::chrono::steady_clock::now() >= deadline)
        stopped = true;
    return stopped;
}

// Best move from the previous iteration, searched first at the root
const Move* rootFirstMove = nullptr;

struct OrderedMove {
    Pos from, to;
    ChessBoard::Piece promotion;
    int order; // higher is searched first
};

// All legal moves, best-looking first, so alpha-beta finds good moves early and cuts more.
// Captures use MVV-LVA (most valuable victim, least valuable attacker): 10 * victim - attacker,
// so pawn takes queen (8900) comes before queen takes pawn (100). Promotions add the new piece's value.
std::vector<OrderedMove> orderedMoves(ChessBoard& board, bool capturesOnly = false, const Move* first = nullptr) {
    std::vector<OrderedMove> moves;
    for (const auto& from : board.moverPieces()) {
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
                int pieceOrder = promotion ? order + ChessBoard::pieceValues[piece] : order;
                if (first && first->from == from.first && first->to == to && first->promotion == piece)
                    pieceOrder = 1 << 30;
                moves.push_back({ from.first, to, piece, pieceOrder });
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
    if (outOfTime())
        return best;
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
Move minimax(ChessBoard& board, int depth, int alpha, int beta, bool minimize, int ply) {
    Move best_move;
    best_move.score = 0;
    if (0 == depth) {
        best_move.score = quiesce(board, alpha, beta, minimize);
        return best_move;
    }
    ++searchNodes;
    if (outOfTime())
        return best_move;
    // A drawn position is worth 0 to both sides, however much material is left
    if (ply > 0 && board.drawReason())
        return best_move;

    for (const OrderedMove& move : orderedMoves(board, false, ply == 0 ? rootFirstMove : nullptr)) {
        ChessBoard branch = board;
        branch.applyMove(move.from, move.to, move.promotion);
        Move option = minimax(branch, depth - 1, alpha, beta, !minimize, ply + 1);
        if (stopped)
            return best_move;
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
    // No legal moves: checkmate or stalemate (a draw, score 0). Being mated scores
    // MATE_SCORE - ply against you, so a quicker mate scores higher for the winner.
    if (best_move.from.x == -1 && board.isKingUnderAttack(board.turn))
        best_move.score = minimize ? MATE_SCORE - ply : -(MATE_SCORE - ply);
    return best_move;
}

// Iterative deepening: search depth 1, 2, 3, ... and keep the deepest finished result.
// Each depth costs roughly 1/35 of the next, so the repeats are cheap, and the previous
// best move is searched first, which makes alpha-beta cut more.
Move searchBestMove(ChessBoard& board, int maxDepth, int timeLimitMs,
    const std::function<void(int depth, const Move& best)>& onDepth) {
    bool minimize = board.turn == ChessBoard::Turn::black; // score() is white minus black
    searchNodes = 0;
    stopped = false;
    timeLimited = false; // depth 1 always finishes, so there is always a move
    deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeLimitMs);

    Move best;
    for (int depth = 1; depth <= maxDepth; ++depth) {
        rootFirstMove = depth > 1 ? &best : nullptr;
        Move result = minimax(board, depth, -INFINITE_SCORE, INFINITE_SCORE, minimize, 0);
        rootFirstMove = nullptr;
        if (stopped)
            break; // unfinished depth: keep the previous result
        best = result;
        if (onDepth)
            onDepth(depth, best);
        // Stop early once a forced mate is found, or when the next depth surely won't finish in time
        if (best.score >= MATE_SCORE - 1000 || best.score <= -(MATE_SCORE - 1000))
            break;
        timeLimited = timeLimitMs > 0;
        if (timeLimited && std::chrono::steady_clock::now() >= deadline)
            break;
    }
    timeLimited = false;
    return best;
}

void stopSearch() {
    stopped = true;
}

Move AIMove(ChessBoard& board, int timeLimitMs) {
    Move m = searchBestMove(board, 64, timeLimitMs);
    board.makeMove(m.from, m.to, m.promotion);
    return m;
}
