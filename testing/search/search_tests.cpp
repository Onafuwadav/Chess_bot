// Search tests: the engine should find obvious good moves for either colour.

#include <catch2/catch_test_macros.hpp>
#include "board.h"
#include "search.h"
#include <string>

namespace {

// Plays the engine's move from the FEN and returns it as text, e.g. "d5e4"
std::string engineMove(const char* fen) {
    ChessBoard board;
    REQUIRE(board.loadFEN(fen));
    Move m = AIMove(board);
    return ChessBoard::toAlgebraic(m.from) + ChessBoard::toAlgebraic(m.to);
}

} // namespace

TEST_CASE("Engine takes a free queen as white", "[search]") {
    CHECK(engineMove("4k3/8/8/3q4/4P3/8/8/4K3 w - - 0 1") == "e4d5");
}

TEST_CASE("Engine takes a free queen as black", "[search]") {
    CHECK(engineMove("4k3/8/8/3p4/4Q3/8/8/4K3 b - - 0 1") == "d5e4");
}

TEST_CASE("Search sees the recapture after its last move", "[search]") {
    // The d5 pawn is defended by e6. A 1-ply search that scores Qxd5 straight away
    // thinks it won a pawn; quiescence search plays on to exd5 and sees the queen is lost.
    ChessBoard board;
    REQUIRE(board.loadFEN("4k3/8/4p3/3p4/8/8/8/3QK3 w - - 0 1"));
    Move m = minimax(board, 1, -INFINITE_SCORE, INFINITE_SCORE, false);
    CHECK(ChessBoard::toAlgebraic(m.from) + ChessBoard::toAlgebraic(m.to) != "d1d5");
}
