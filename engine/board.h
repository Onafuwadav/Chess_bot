#pragma once
#include <map>
#include <string>

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
    enum class CheckStatus { None, Check, Checkmate, Stalemate };
    enum class Turn { white, black } turn;
    enum class Piece { king, queen, white_pawn, black_pawn, rook, bishop, knight }; // Defining chess piece types
    static std::map<Piece, int> pieceValues; // Map to hold values for each piece and their values (defined in eval.cpp)

    Pos LastMoveTo;
    Pos LastMoveFrom;
    Piece LastMovePiece = Piece::king; // Piece that made the last move (en passant needs to know it was a pawn)
    std::map<Pos, Piece> white_pieces, black_pieces; // Maps to store positions of pieces for each player

    // Castling rights as bit flags (lost when the king or that rook moves, or the rook is captured)
    enum Castle { WhiteKingSide = 1, WhiteQueenSide = 2, BlackKingSide = 4, BlackQueenSide = 8 };
    int castlingRights = 0;
    std::map<Pos, Piece>& moverPieces() { return turn == Turn::white ? white_pieces : black_pieces; } // Function to retrieve pieces of the current player
    std::map<Pos, Piece>& opponentPieces() { return turn == Turn::white ? black_pieces : white_pieces; } // Function to retrieve pieces of the opponent player

    // Function to reset the chessboard to initial state
    void reset();

    // Sets up the board from a FEN string, e.g. the start position is
    // "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1". Returns false if it can't be parsed.
    bool loadFEN(const std::string& fen);

    // Board coordinates: y is the rank, and x = 1 is the h-file up to x = 8 for the a-file
    // (printBoard shows the board from black's side). These convert to and from squares like "e4".
    static Pos fromAlgebraic(const std::string& square);
    static std::string toAlgebraic(const Pos& p);

    // Switches turn between black and white player after turn end
    void flipTurn() { turn = turn == Turn::white ? Turn::black : Turn::white; }

    // Function to execute move on board(checking first if move is possible and legal) then switching turn.
    // 'promotion' is the piece a pawn becomes when it reaches the last rank.
    bool makeMove(Pos from, Pos to, Piece promotion = Piece::queen);

    // Plays the move without checking it is allowed (used by makeMove and by the legality check)
    void applyMove(Pos from, Pos to, Piece promotion = Piece::queen);

    // True if any piece of side 'by' attacks the square
    bool isSquareAttacked(const Pos& square, Turn by) const;

    // True if whichKing's king is in check
    bool isKingUnderAttack(Turn whichKing) const;
    // True if the side to move has at least one legal move
    bool hasLegalMove();

    // Check, checkmate or stalemate for the side to move
    CheckStatus StatusCheck();
};
