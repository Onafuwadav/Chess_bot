#include "board.h"
#include "eval.h"
#include "search.h"
#include <cctype>
#include <iostream>
#include <map>
#include <string>

using Piece = ChessBoard::Piece;
using Turn = ChessBoard::Turn;

static bool show_coordinates = false;

void printBoard(ChessBoard& board) {
    static std::map<Piece, char> sprites =
    { {Piece::white_pawn,'P'}, {Piece::black_pawn,'P'}, {Piece::rook,'R'}, {Piece::knight,'N'},
       {Piece::bishop,'B'}, {Piece::king,'K'}, {Piece::queen,'Q'} };
    std::cout << std::endl << "        1     2     3     4     5     6     7     8   " << std::endl;
    std::cout << "      _____ _____ _____ _____ _____ _____ _____ _____ ";
    for (int y = 1; y < 9; ++y) {
        if (show_coordinates)
            std::cout << std::endl << "     |1" << y << "   |2" << y << "   |3" << y << "   |4" << y << "   |5" << y << "   |6" << y << "   |7" << y << "   |8" << y << "   |";
        else
            std::cout << std::endl << "     |     |     |     |     |     |     |     |     |";
        std::cout << std::endl << "  " << y << "  ";
        for (int x = 1; x < 9; ++x) {
            std::cout << "|  ";
            if (board.white_pieces.count(Pos(x, y)))
                std::cout << sprites[board.white_pieces[Pos(x, y)]];
            else if (board.black_pieces.count(Pos(x, y)))
                std::cout << (char)tolower(sprites[board.black_pieces[Pos(x, y)]]);
            else
                std::cout << " ";
            std::cout << "  ";
        }
        std::cout << "|  " << y << std::endl << "     |_____|_____|_____|_____|_____|_____|_____|_____|";
    }
    std::cout << std::endl << std::endl << "        1     2     3     4     5     6     7     8   " << std::endl << std::endl;
    int result = score(board);
    std::cout << "The result is: " << result << std::endl << std::endl;
}

void printHelp() {
    std::cout << std::endl << "* h: help, q: quit, p: show board, c: toggle show coordinates inside squares" << std::endl << "* Input format: yxyx is from-to coordinates, e.g: 1715 moves (x,y)=(1,7) to (x,y)=(1,5)" << std::endl << std::endl;
}

/* False to exit */
bool promptInput(ChessBoard& board) {
    std::string move;
illegalmove:
    if (board.turn == Turn::white){
        std::cout << "White move: ";
    }
    else
        std::cout << "Black move: ";
    if (move == "")
        std::cin >> move;
    if (move == "q") {
        std::cout << "Good bye" << std::endl << std::endl;
        return false;
    }
    if (move == "?" || move == "h" || move == "help") {
        printHelp();
        move = "";
        goto illegalmove;
    }
    if (move == "c") {
        show_coordinates = !show_coordinates;
        printBoard(board);
        move = "";
        goto illegalmove;
    }

    if (move == "p") {
        printBoard(board);
        move = "";
        goto illegalmove;
    }
    Pos from(-1, -1), to(-1, -1);
    if (move.length() == 4) {
        from.x = move[0] - '0';
        from.y = move[1] - '0';
        to.x = move[2] - '0';
        to.y = move[3] - '0';
    }
    if (!board.makeMove(from, to)) {
        std::cout << "* Illegal move" << std::endl;
        move = "";
        goto illegalmove;
    }
    printBoard(board);
    return true;
}

void playAIMove(ChessBoard& board) {
    AIMove(board);
    printBoard(board);
}

int main() {
    ChessBoard game;
    std::cout << std::endl << "* Chesscomputer v.0.1" << std::endl;
    printHelp();
    bool gameon = true;
    while (gameon) {
        game.reset();
        std::string pick_side = "";
        while (pick_side != "b" && pick_side != "w" && pick_side != "q") {
            std::cout << std::endl << "Play as (b)lack or (w)hite or (q)uit? ";
            std::cin >> pick_side;
        }
        if (pick_side == "q") {
            std::cout << "Good bye." << std::endl;
            break;
        }

        if (pick_side == "b")
            playAIMove(game);
        else
            printBoard(game);

        while ((gameon = promptInput(game))) {
            if (!game.hasKing()) {
                std::cout << "* You are victorious!" << std::endl;
                break;
            }
            playAIMove(game);
            if (!game.hasKing()) {
                std::cout << "* You are defeated!" << std::endl;
                break;
            }
        }
    }
    return 0;
}
