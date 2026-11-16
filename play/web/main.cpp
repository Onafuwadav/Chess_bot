// Browser front-end compiled to WebAssembly with Emscripten
//
// Search is single thread so the page and board stay responsive while thinking

#include "board.h"
#include "movegen.h"
#include "perft.h"
#include "search.h"
#include <sstream>
#include <string>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

namespace {

const char* kStartFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

ChessBoard board;
std::string reply; 

// Plays move given as text returns false if is illegal
bool playMoveText(const std::string& text) {
    if (text.size() < 4)
        return false;
    Pos from = ChessBoard::fromAlgebraic(text.substr(0, 2));
    Pos to = ChessBoard::fromAlgebraic(text.substr(2, 2));
    ChessBoard::Piece promotion = ChessBoard::Piece::queen;
    if (text.size() > 4) {
        switch (text[4]) {
        case 'r': promotion = ChessBoard::Piece::rook; break;
        case 'b': promotion = ChessBoard::Piece::bishop; break;
        case 'n': promotion = ChessBoard::Piece::knight; break;
        default: break;
        }
    }
    return board.makeMove(from, to, promotion);
}

std::string moveText(const Pos& from, const Pos& to, ChessBoard::Piece promotion) {
    std::string text = ChessBoard::toAlgebraic(from) + ChessBoard::toAlgebraic(to);
    if (isPromotion(board, from, to))
        text += promotionLetter(promotion);
    return text;
}

} // namespace

extern "C" {

// Returns 1 if legal, 0 if FEN or move rejected
EMSCRIPTEN_KEEPALIVE int chess_position(const char* fen, const char* moves) {
    std::string start = fen && *fen ? fen : kStartFEN;
    if (!board.loadFEN(start)) {
        board.loadFEN(kStartFEN);
        return 0;
    }
    std::istringstream in(moves ? moves : "");
    std::string move;
    while (in >> move)
        if (!playMoveText(move))
            return 0;
    return 1;
}

EMSCRIPTEN_KEEPALIVE const char* chess_bestmove(int timeMs, int maxDepth) {
    ChessBoard position = board;
    Move best = searchBestMove(position, maxDepth > 0 ? maxDepth : 64, timeMs);
    reply = best.from.x == -1 ? "0000" : moveText(best.from, best.to, best.promotion);
    return reply.c_str();
}

// Every legal move for the side to move, separated by spaces
EMSCRIPTEN_KEEPALIVE const char* chess_legalmoves() {
    reply.clear();
    for (const auto& from : board.moverPieces())
        for (auto& to : possibleMoves(board, from.first, board.LastMoveTo, board.LastMoveFrom, board.LastMovePiece)) {
            bool promotion = isPromotion(board, from.first, to);
            for (ChessBoard::Piece piece : promotionPieces) {
                if (!reply.empty())
                    reply += ' ';
                reply += moveText(from.first, to, piece);
                if (!promotion)
                    break;
            }
        }
    return reply.c_str();
}

// "white" or "black" to move, then the state: "none", "check", "checkmate", "stalemate", "draw"
    reply = board.turn == ChessBoard::Turn::white ? "white " : "black ";
    switch (board.StatusCheck()) {
    case ChessBoard::CheckStatus::None: reply += "none"; break;
    case ChessBoard::CheckStatus::Check: reply += "check"; break;
    case ChessBoard::CheckStatus::Checkmate: reply += "checkmate"; break;
    case ChessBoard::CheckStatus::Stalemate: reply += "stalemate"; break;
    case ChessBoard::CheckStatus::Draw: {
        const char* reason = board.drawReason();
        reply += std::string("draw") + (reason ? std::string(" ") + reason : "");
        break;
    }
    }
    return reply.c_str();
}

EMSCRIPTEN_KEEPALIVE double chess_perft(int depth) {
    ChessBoard position = board;
    return static_cast<double>(perft(position, depth));
}

// Positions visited by the last chess_bestmove
EMSCRIPTEN_KEEPALIVE double chess_nodes() {
    return static_cast<double>(searchNodes);
}

} // extern "C"
