// Evaluation tests: the score should be symmetric between the colours.

#include <catch2/catch_test_macros.hpp>
#include "board.h"
#include "eval.h"

namespace {

int scoreOf(const char* fen) {
    ChessBoard board;
    REQUIRE(board.loadFEN(fen));
    return score(board);
}

} // namespace

TEST_CASE("Start position scores 0", "[eval]") {
    CHECK(scoreOf("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") == 0);
}

TEST_CASE("Mirrored position has the opposite score", "[eval]") {
    // Same position with the colours swapped and the board flipped
    int original = scoreOf("r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3");
    int mirrored = scoreOf("rnbqkb1r/pppp1ppp/5n2/4p3/4P3/2N5/PPPP1PPP/R1BQKBNR b KQkq - 2 3");
    CHECK(original == -mirrored);

    int endgame = scoreOf("8/5pk1/6p1/3R4/1r6/6P1/5PK1/8 w - - 0 1");
    int endgameMirrored = scoreOf("8/5pk1/6p1/1R6/3r4/6P1/5PK1/8 b - - 0 1");
    CHECK(endgame == -endgameMirrored);
}

TEST_CASE("Piece-square tables prefer central knights and advanced pawns", "[eval]") {
    // Knight on e4 vs knight on a1 (kings identical)
    CHECK(scoreOf("4k3/8/8/8/4N3/8/8/4K3 w - - 0 1") > scoreOf("4k3/8/8/8/8/8/8/N3K3 w - - 0 1"));
    // Pawn on e6 vs pawn on e3
    CHECK(scoreOf("4k3/8/4P3/8/8/8/8/4K3 w - - 0 1") > scoreOf("4k3/8/8/8/8/4P3/8/4K3 w - - 0 1"));
}
