// Search tests: the engine should find obvious good moves for either colour.

#include <catch2/catch_test_macros.hpp>
#include "board.h"
#include "search.h"
#include <chrono>
#include <string>

namespace {

// Plays the engine's move from the FEN and returns it as text, e.g. "d5e4"
std::string engineMove(const char* fen) {
    ChessBoard board;
    REQUIRE(board.loadFEN(fen));
    Move m = searchBestMove(board, 4, 0);
    return ChessBoard::toAlgebraic(m.from) + ChessBoard::toAlgebraic(m.to);
}

} // namespace

TEST_CASE("Engine takes a free queen as white", "[search]") {
    CHECK(engineMove("4k3/8/8/3q4/4P3/8/8/4K3 w - - 0 1") == "e4d5");
}

TEST_CASE("Engine takes a free queen as black", "[search]") {
    CHECK(engineMove("4k3/8/8/3p4/4Q3/8/8/4K3 b - - 0 1") == "d5e4");
}

TEST_CASE("Engine finds mate in one and scores it as mate", "[search]") {
    // Back-rank mate: Ra8#
    ChessBoard board;
    REQUIRE(board.loadFEN("6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1"));
    Move m = searchBestMove(board, 4, 0);
    CHECK(ChessBoard::toAlgebraic(m.from) + ChessBoard::toAlgebraic(m.to) == "a1a8");
    CHECK(m.score == MATE_SCORE - 1);
}

TEST_CASE("Engine prefers the quickest mate", "[search]") {
    // Qb7# mates at once; other queen moves mate later
    ChessBoard board;
    REQUIRE(board.loadFEN("k7/8/1K6/8/8/8/8/7Q w - - 0 1"));
    Move m = searchBestMove(board, 4, 0);
    board.makeMove(m.from, m.to, m.promotion);
    CHECK(board.StatusCheck() == ChessBoard::CheckStatus::Checkmate);
}

TEST_CASE("Search stops at the time limit", "[search]") {
    ChessBoard board;
    REQUIRE(board.loadFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"));
    auto start = std::chrono::steady_clock::now();
    Move m = searchBestMove(board, 64, 300);
    double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    CHECK(m.from.x != -1);
    CHECK(ms < 600);
}

TEST_CASE("Search sees the recapture after its last move", "[search]") {
    // The d5 pawn is defended by e6. A 1-ply search that scores Qxd5 straight away
    // thinks it won a pawn; quiescence search plays on to exd5 and sees the queen is lost.
    ChessBoard board;
    REQUIRE(board.loadFEN("4k3/8/4p3/3p4/8/8/8/3QK3 w - - 0 1"));
    Move m = minimax(board, 1, -INFINITE_SCORE, INFINITE_SCORE, false);
    CHECK(ChessBoard::toAlgebraic(m.from) + ChessBoard::toAlgebraic(m.to) != "d1d5");
}
