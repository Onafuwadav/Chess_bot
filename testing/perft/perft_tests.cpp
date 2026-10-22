// Perft tests: count every position reachable to a given depth and compare with
// the published counts from https://www.chessprogramming.org/Perft_Results
//
// Tests tagged [!shouldfail] are known to be wrong right now. Catch2 reports
// them as passing while they fail, and as failing once they pass, so each fix
// has to remove its tag.
//
// The deeper counts are tagged [.slow] and only run when asked for:
//   perft_tests "[slow]"

#include <catch2/catch_test_macros.hpp>
#include "board.h"
#include "perft.h"
#include <initializer_list>

namespace {

const char* kStartPos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
const char* kKiwipete = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
const char* kPosition3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";
const char* kPosition4 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
const char* kPosition5 = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
const char* kPosition6 = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";

// Checks perft(1), perft(2), ... against the expected counts in order
void checkPerft(const char* fen, std::initializer_list<uint64_t> expected, int firstDepth = 1) {
    ChessBoard board;
    REQUIRE(board.loadFEN(fen));
    int depth = firstDepth;
    for (uint64_t nodes : expected) {
        INFO("depth " << depth);
        CHECK(perft(board, depth) == nodes);
        ++depth;
    }
}

} // namespace

TEST_CASE("FEN start position matches reset()", "[fen]") {
    ChessBoard fromReset, fromFEN;
    fromReset.reset();
    REQUIRE(fromFEN.loadFEN(kStartPos));
    CHECK(fromReset.white_pieces == fromFEN.white_pieces);
    CHECK(fromReset.black_pieces == fromFEN.black_pieces);
    CHECK(fromReset.turn == fromFEN.turn);
}

TEST_CASE("Square names convert both ways", "[fen]") {
    CHECK(ChessBoard::fromAlgebraic("e1") == Pos(4, 1));
    CHECK(ChessBoard::fromAlgebraic("a8") == Pos(8, 8));
    CHECK(ChessBoard::toAlgebraic(Pos(1, 1)) == "h1");
    CHECK(ChessBoard::toAlgebraic(ChessBoard::fromAlgebraic("c6")) == "c6");
}

TEST_CASE("Perft: start position", "[perft]") {
    checkPerft(kStartPos, { 20, 400, 8902, 197281 });
}

TEST_CASE("Perft: Kiwipete", "[perft][!shouldfail]") {
    checkPerft(kKiwipete, { 48, 2039, 97862 });
}

TEST_CASE("Perft: position 3", "[perft][!shouldfail]") {
    checkPerft(kPosition3, { 14, 191, 2812, 43238 });
}

TEST_CASE("Perft: position 4", "[perft][!shouldfail]") {
    checkPerft(kPosition4, { 6, 264, 9467 });
}

TEST_CASE("Perft: position 5", "[perft][!shouldfail]") {
    checkPerft(kPosition5, { 44, 1486, 62379 });
}

TEST_CASE("Perft: position 6", "[perft]") {
    checkPerft(kPosition6, { 46, 2079, 89890 });
}

TEST_CASE("Perft deep: start position", "[perft][.slow][!shouldfail]") {
    checkPerft(kStartPos, { 4865609 }, 5);
}

TEST_CASE("Perft deep: Kiwipete", "[perft][.slow][!shouldfail]") {
    checkPerft(kKiwipete, { 4085603 }, 4);
}

TEST_CASE("Perft deep: position 3", "[perft][.slow][!shouldfail]") {
    checkPerft(kPosition3, { 674624 }, 5);
}

TEST_CASE("Perft deep: position 4", "[perft][.slow][!shouldfail]") {
    checkPerft(kPosition4, { 422333 }, 4);
}

TEST_CASE("Perft deep: position 5", "[perft][.slow][!shouldfail]") {
    checkPerft(kPosition5, { 2103487 }, 4);
}

TEST_CASE("Perft deep: position 6", "[perft][.slow]") {
    checkPerft(kPosition6, { 3894594 }, 4);
}
