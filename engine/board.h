#pragma once
#include <map>

struct Pos {
    int x, y;

    // Constructor taking another Pos object and optional offsets for x and y
    Pos(const Pos& p, int dx = 0, int dy = 0) {
        *this = p;  // Copy the position 'p'
        x += dx;    // Apply optional x offset
        y += dy;    // Apply optional y offset
    }

    // Constructor with explicit x and y values
    Pos(int _x, int _y) {
        x = _x;  // Initialize x coordinate
        y = _y;  // Initialize y coordinate
    }

    Pos& operator=(const Pos& p) = default;

    // Operator overloading for less than comparison
    bool operator<(const Pos& p) const {
        return (x < p.x) || (x == p.x && y < p.y); // Compare positions based on x and y values
    }

    // Operator overloading for equality comparison
    bool operator==(const Pos& p) const {
        return x == p.x && y == p.y; // Check if positions are equal
    }

    // Default constructor initializing x and y to -1
    Pos() {
        x = -1;
        y = -1;
    }
};

struct ChessBoard {
    enum class CheckStatus { None, Check, Checkmate };
    enum class Turn { white, black } turn;
    enum class Piece { king, queen, white_pawn, black_pawn, rook, bishop, knight }; // Defining chess piece types
    static std::map<Piece, int> pieceValues; // Map to hold values for each piece and their values (defined in eval.cpp)

    Pos LastMoveTo;
    Pos LastMoveFrom;
    std::map<Pos, Piece> LastMove; // Saving last move made
    std::map<Pos, Piece> white_pieces, black_pieces; // Maps to store positions of pieces for each player
    std::map<Pos, Piece>& moverPieces() { return turn == Turn::white ? white_pieces : black_pieces; } // Function to retrieve pieces of the current player
    std::map<Pos, Piece>& opponentPieces() { return turn == Turn::white ? black_pieces : white_pieces; } // Function to retrieve pieces of the opponent player

    // Function to reset the chessboard to initial state
    void reset();

    // Switches turn between black and white player after turn end
    void flipTurn() { turn = turn == Turn::white ? Turn::black : Turn::white; }

    // Function to execute move on board(checking first if move is possible and legal) then switching turn
    bool makeMove(Pos from, Pos to);

    bool isKingUnderAttack(ChessBoard& board, Turn whichKing);
    CheckStatus StatusCheck(ChessBoard& board);

    // add check/checkmate function
    bool hasKing();
};
