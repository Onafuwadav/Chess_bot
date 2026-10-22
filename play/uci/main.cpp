// UCI (Universal Chess Interface) front-end, so the engine can run inside chess GUIs
// such as ChessX or Cute Chess, and play matches with cutechess-cli.
//
// Supported: uci, isready, ucinewgame, position (startpos | fen ...) [moves ...],
// go (wtime btime winc binc movestogo movetime depth infinite | perft N), stop, quit.

#include "board.h"
#include "movegen.h"
#include "perft.h"
#include "search.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

namespace {

const char* kStartFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

std::mutex outputMutex;

// The search thread and the input thread both print, so lines are written one at a time
void say(const std::string& line) {
    std::lock_guard<std::mutex> lock(outputMutex);
    std::cout << line << std::endl;
}

// Move text such as "e2e4" or "e7e8q"
std::string moveText(const ChessBoard& board, const Move& m) {
    if (m.from.x == -1)
        return "0000";
    std::string text = ChessBoard::toAlgebraic(m.from) + ChessBoard::toAlgebraic(m.to);
    if (isPromotion(board, m.from, m.to))
        text += promotionLetter(m.promotion);
    return text;
}

// Plays a move given as text; false if it isn't legal here
bool playMoveText(ChessBoard& board, const std::string& text) {
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

// UCI scores are from the side to move's point of view, in centipawns or "mate N" (N in moves)
std::string scoreText(const ChessBoard& board, int whiteScore) {
    int score = board.turn == ChessBoard::Turn::white ? whiteScore : -whiteScore;
    if (score >= MATE_SCORE - 1000)
        return "mate " + std::to_string((MATE_SCORE - score + 1) / 2);
    if (score <= -(MATE_SCORE - 1000))
        return "mate -" + std::to_string((MATE_SCORE + score + 1) / 2);
    return "cp " + std::to_string(score);
}

// "position startpos moves e2e4 e7e5" or "position fen <6 fields> moves ..."
void handlePosition(ChessBoard& board, std::istringstream& in) {
    std::string token, fen;
    in >> token;
    if (token == "startpos") {
        fen = kStartFEN;
        in >> token; // "moves" or nothing
    } else if (token == "fen") {
        while (in >> token && token != "moves")
            fen += token + " ";
    } else {
        return;
    }
    board.loadFEN(fen);
    while (in >> token)
        if (token != "moves" && !playMoveText(board, token)) {
            say("info string illegal move " + token);
            break;
        }
}

// How long to think: all of movetime, or a slice of the remaining clock
int thinkingTimeMs(const ChessBoard& board, int wtime, int btime, int winc, int binc, int movestogo, int movetime) {
    if (movetime > 0)
        return movetime > 60 ? movetime - 30 : movetime / 2; // leave a little for overhead
    bool white = board.turn == ChessBoard::Turn::white;
    int timeLeft = white ? wtime : btime;
    int increment = white ? winc : binc;
    if (timeLeft <= 0)
        return 0; // no clock given: no limit (depth or "stop" ends the search)
    int movesLeft = movestogo > 0 ? movestogo : 30;
    int ms = timeLeft / movesLeft + increment * 3 / 4;
    // Never use more than a third of what is left, and keep 50 ms in reserve
    if (ms > timeLeft / 3) ms = timeLeft / 3;
    if (ms > timeLeft - 50) ms = timeLeft - 50;
    return ms > 1 ? ms : 1;
}

} // namespace

int main() {
    ChessBoard board;
    board.loadFEN(kStartFEN);

    std::thread searchThread;
    std::atomic<bool> searching{ false };

    // Ends a running search and waits for it to print its bestmove
    auto finishSearch = [&]() {
        if (searchThread.joinable()) {
            while (searching) {
                stopSearch();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            searchThread.join();
        }
    };

    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string command;
        in >> command;

        if (command == "uci") {
            say("id name Chess_bot");
            say("id author David Onafuwa");
            say("uciok");
        } else if (command == "isready") {
            say("readyok");
        } else if (command == "ucinewgame") {
            finishSearch();
            board.loadFEN(kStartFEN);
        } else if (command == "position") {
            finishSearch();
            handlePosition(board, in);
        } else if (command == "go") {
            finishSearch();
            int wtime = 0, btime = 0, winc = 0, binc = 0, movestogo = 0, movetime = 0, depth = 64;
            std::string token;
            while (in >> token) {
                if (token == "wtime") in >> wtime;
                else if (token == "btime") in >> btime;
                else if (token == "winc") in >> winc;
                else if (token == "binc") in >> binc;
                else if (token == "movestogo") in >> movestogo;
                else if (token == "movetime") in >> movetime;
                else if (token == "depth") in >> depth;
                else if (token == "perft") {
                    int perftDepth = 1;
                    in >> perftDepth;
                    uint64_t total = 0;
                    for (auto& entry : perftDivide(board, perftDepth)) {
                        say(entry.first + ": " + std::to_string(entry.second));
                        total += entry.second;
                    }
                    say("Nodes searched: " + std::to_string(total));
                    depth = -1;
                }
            }
            if (depth < 0)
                continue; // was "go perft"

            int timeMs = thinkingTimeMs(board, wtime, btime, winc, binc, movestogo, movetime);
            searching = true;
            ChessBoard position = board;
            searchThread = std::thread([position, depth, timeMs, &searching]() mutable {
                auto start = std::chrono::steady_clock::now();
                Move best = searchBestMove(position, depth, timeMs, [&](int d, const Move& m) {
                    long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - start).count();
                    long long nps = ms > 0 ? (long long)(searchNodes * 1000 / ms) : 0;
                    say("info depth " + std::to_string(d) + " score " + scoreText(position, m.score)
                        + " nodes " + std::to_string(searchNodes) + " nps " + std::to_string(nps)
                        + " time " + std::to_string(ms) + " pv " + moveText(position, m));
                });
                say("bestmove " + moveText(position, best));
                searching = false;
            });
        } else if (command == "stop") {
            finishSearch();
        } else if (command == "quit") {
            break;
        }
    }
    finishSearch();
    return 0;
}
