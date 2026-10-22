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
