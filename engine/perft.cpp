#include "perft.h"
#include "movegen.h"

uint64_t perft(ChessBoard& board, int depth) {
    if (depth == 0)
        return 1;

    uint64_t nodes = 0;
    for (const auto& from : board.moverPieces()) {
        std::vector<Pos> moves = possibleMoves(board, from.first, board.LastMoveTo, board.LastMoveFrom, board.LastMovePiece);
        for (auto& to : moves) {
            bool promotion = isPromotion(board, from.first, to);
            // At the last ply each move is one position, so count them without playing them
            if (depth == 1) {
                nodes += promotion ? 4 : 1;
                continue;
            }
            for (ChessBoard::Piece piece : promotionPieces) {
                ChessBoard branch = board;
                branch.applyMove(from.first, to, piece);
                nodes += perft(branch, depth - 1);
                if (!promotion)
                    break;
            }
        }
    }
    return nodes;
}

std::vector<std::pair<std::string, uint64_t>> perftDivide(ChessBoard& board, int depth) {
    std::vector<std::pair<std::string, uint64_t>> result;
    for (const auto& from : board.moverPieces()) {
        for (auto& to : possibleMoves(board, from.first, board.LastMoveTo, board.LastMoveFrom, board.LastMovePiece)) {
            bool promotion = isPromotion(board, from.first, to);
            for (ChessBoard::Piece piece : promotionPieces) {
                ChessBoard branch = board;
                branch.applyMove(from.first, to, piece);
                std::string name = ChessBoard::toAlgebraic(from.first) + ChessBoard::toAlgebraic(to);
                if (promotion)
                    name += promotionLetter(piece);
                result.emplace_back(name, perft(branch, depth - 1));
                if (!promotion)
                    break;
            }
        }
    }
    return result;
}
