// Search benchmark: searches a few fixed positions to a fixed depth and prints
// the move found, positions searched and time taken.
//   engine_bench [depth]   (default 3)

#include "board.h"
#include "search.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv) {
    int depth = argc > 1 ? std::atoi(argv[1]) : 3;

    const char* positions[][2] = {
        { "start", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" },
        { "kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1" },
        { "middlegame", "r1bq1rk1/pp2bppp/2n1pn2/3p4/2PP4/2N1PN2/PP1B1PPP/R2QKB1R w KQ - 0 8" },
        { "endgame", "8/5pk1/6p1/3R4/1r6/6P1/5PK1/8 w - - 0 1" },
    };

    uint64_t totalNodes = 0;
    double totalSeconds = 0;
    std::printf("depth %d\n%-12s %-6s %12s %10s %12s\n", depth, "position", "move", "nodes", "ms", "nodes/s");
    for (auto& p : positions) {
        ChessBoard board;
        board.loadFEN(p[1]);
        searchNodes = 0;
        auto start = std::chrono::steady_clock::now();
        Move m = minimax(board, depth, -INFINITE_SCORE, INFINITE_SCORE, board.turn == ChessBoard::Turn::black);
        double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        std::string move = ChessBoard::toAlgebraic(m.from) + ChessBoard::toAlgebraic(m.to);
        std::printf("%-12s %-6s %12llu %10.0f %12.0f\n", p[0], move.c_str(),
            (unsigned long long)searchNodes, seconds * 1000, searchNodes / seconds);
        totalNodes += searchNodes;
        totalSeconds += seconds;
    }
    std::printf("%-12s %-6s %12llu %10.0f %12.0f\n", "total", "", (unsigned long long)totalNodes,
        totalSeconds * 1000, totalNodes / totalSeconds);
    return 0;
}
