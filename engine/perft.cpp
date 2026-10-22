#include "perft.h"
#include "movegen.h"

uint64_t perft(ChessBoard& board, int depth) {
    if (depth == 0)
        return 1;

    uint64_t nodes = 0;
    for (auto& from : board.moverPieces()) {
        std::vector<Pos> moves = possibleMoves(board, from.first, board.LastMoveTo, board.LastMoveFrom, board.LastMove[board.LastMoveTo]);
        // At the last ply each move is one position, so count them without playing them
        if (depth == 1) {
            nodes += moves.size();
            continue;
        }
        for (auto& to : moves) {
            ChessBoard branch = board;
            branch.makeMove(from.first, to);
            nodes += perft(branch, depth - 1);
        }
    }
    return nodes;
}

std::vector<std::pair<std::string, uint64_t>> perftDivide(ChessBoard& board, int depth) {
    std::vector<std::pair<std::string, uint64_t>> result;
    for (auto& from : board.moverPieces()) {
        for (auto& to : possibleMoves(board, from.first, board.LastMoveTo, board.LastMoveFrom, board.LastMove[board.LastMoveTo])) {
            ChessBoard branch = board;
            branch.makeMove(from.first, to);
            result.emplace_back(ChessBoard::toAlgebraic(from.first) + ChessBoard::toAlgebraic(to), perft(branch, depth - 1));
        }
    }
    return result;
}
