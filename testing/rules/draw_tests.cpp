// Draw rules: 50-move rule, threefold repetition, insufficient material.

#include <catch2/catch_test_macros.hpp>
#include "board.h"
#include <cstring>

namespace {

void play(ChessBoard& board, const char* from, const char* to) {
    REQUIRE(board.makeMove(ChessBoard::fromAlgebraic(from), ChessBoard::fromAlgebraic(to)));
}

} // namespace

TEST_CASE("Same position reached by different move orders has the same key", "[draw]") {
    ChessBoard a, b;
    a.reset();
    b.reset();
    play(a, "g1", "f3"); play(a, "g8", "f6"); play(a, "b1", "c3");
    play(b, "b1", "c3"); play(b, "g8", "f6"); play(b, "g1", "f3");
    CHECK(a.positionKey() == b.positionKey());

    ChessBoard c;
    c.reset();
    CHECK(a.positionKey() != c.positionKey());
}

TEST_CASE("Threefold repetition is a draw", "[draw]") {
    ChessBoard board;
    board.reset();
    for (int i = 0; i < 2; ++i) {
        CHECK(board.StatusCheck() == ChessBoard::CheckStatus::None);
        play(board, "g1", "f3"); play(board, "g8", "f6");
        play(board, "f3", "g1"); play(board, "f6", "g8");
    }
    // The start position has now appeared three times
    CHECK(board.StatusCheck() == ChessBoard::CheckStatus::Draw);
    CHECK(std::strcmp(board.drawReason(), "threefold repetition") == 0);
}

TEST_CASE("A pawn move resets repetition and the 50-move count", "[draw]") {
    ChessBoard board;
    REQUIRE(board.loadFEN("4k3/8/8/8/8/8/4P3/R3K3 w - - 99 80"));
    play(board, "e2", "e3");
    CHECK(board.halfmoveClock == 0);
    CHECK(board.drawReason() == nullptr);
}

TEST_CASE("50-move rule is a draw", "[draw]") {
    ChessBoard board;
    REQUIRE(board.loadFEN("4k3/8/8/8/8/8/8/R3K3 w - - 99 80"));
    CHECK(board.drawReason() == nullptr);
    play(board, "a1", "a2");
    CHECK(board.StatusCheck() == ChessBoard::CheckStatus::Draw);
    CHECK(std::strcmp(board.drawReason(), "50-move rule") == 0);
}

TEST_CASE("Insufficient material is a draw", "[draw]") {
    ChessBoard board;
    REQUIRE(board.loadFEN("4k3/8/8/8/8/8/8/4K3 w - - 0 1"));
    CHECK(board.StatusCheck() == ChessBoard::CheckStatus::Draw);
    REQUIRE(board.loadFEN("4k3/8/8/8/8/8/8/2B1K3 w - - 0 1"));
    CHECK(board.StatusCheck() == ChessBoard::CheckStatus::Draw);
    REQUIRE(board.loadFEN("4k3/8/8/8/8/8/8/1N2K3 b - - 0 1"));
    CHECK(board.StatusCheck() == ChessBoard::CheckStatus::Draw);

    // A rook or a pawn can still mate
    REQUIRE(board.loadFEN("4k3/8/8/8/8/8/8/R3K3 w - - 0 1"));
    CHECK(board.drawReason() == nullptr);
    REQUIRE(board.loadFEN("4k3/8/8/8/8/8/4P3/4K3 w - - 0 1"));
    CHECK(board.drawReason() == nullptr);
}
