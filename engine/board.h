#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

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

// What's on each of the 64 squares, for one colour. It works like the std::map<Pos, Piece>
// it replaced (count, [], erase, find, and iterating gives {first = Pos, second = piece} in
// the same order), but it is a fixed array: no memory allocation, and copying it is a
// 72-byte copy. A bit per square in 'occupied' says which squares hold a piece.
template <typename PieceType>
class SquareMap {
public:
    struct Entry { Pos first; PieceType second; };

    class iterator {
    public:
        iterator(const SquareMap* map, uint64_t remaining) : map(map), remaining(remaining) {}
        Entry operator*() const { int i = lowestBit(); return { Pos(i / 8 + 1, i % 8 + 1), map->cells[i] }; }
        struct Arrow { Entry entry; const Entry* operator->() const { return &entry; } };
        Arrow operator->() const { return { **this }; }
        iterator& operator++() { remaining &= remaining - 1; return *this; } // clear the lowest set bit
        bool operator!=(const iterator& other) const { return remaining != other.remaining; }
        bool operator==(const iterator& other) const { return remaining == other.remaining; }
    private:
        int lowestBit() const { int i = 0; while (!((remaining >> i) & 1)) ++i; return i; }
        const SquareMap* map;
        uint64_t remaining;
    };

    iterator begin() const { return iterator(this, occupied); }
    iterator end() const { return iterator(this, 0); }

    int count(const Pos& p) const { return onBoard(p) && ((occupied >> index(p)) & 1); }

    iterator find(const Pos& p) const {
        return count(p) ? iterator(this, occupied & ~((uint64_t(1) << index(p)) - 1)) : end();
    }

    // Like std::map: an empty square is filled with the default piece (value 0) before returning it
    PieceType& operator[](const Pos& p) {
        int i = index(p);
        if (!((occupied >> i) & 1)) {
            occupied |= uint64_t(1) << i;
            cells[i] = PieceType();
        }
        return cells[i];
    }

    void erase(const Pos& p) {
        if (onBoard(p))
            occupied &= ~(uint64_t(1) << index(p));
    }

    void clear() { occupied = 0; }

    bool operator==(const SquareMap& other) const {
        if (occupied != other.occupied) return false;
        for (int i = 0; i < 64; ++i)
            if (((occupied >> i) & 1) && cells[i] != other.cells[i]) return false;
        return true;
    }

private:
    // Same order as std::map<Pos> used: by x, then y
    static int index(const Pos& p) { return (p.x - 1) * 8 + (p.y - 1); }
    static bool onBoard(const Pos& p) { return p.x >= 1 && p.x <= 8 && p.y >= 1 && p.y <= 8; }

    uint64_t occupied = 0;
    PieceType cells[64];
};

// Position keys since the last capture or pawn move, in a fixed array (no allocation when the
// board is copied). 102 is enough: the 50-move rule ends the game after 100 half-moves.
class PositionHistory {
public:
    void assign(int, uint64_t key) { size = 1; keys[0] = key; }
    void clear() { size = 0; }
    void push_back(uint64_t key) {
        if (size == kCapacity) { // drop the oldest
            for (int i = 1; i < size; ++i) keys[i - 1] = keys[i];
            --size;
        }
        keys[size++] = key;
    }
    bool empty() const { return size == 0; }
    uint64_t back() const { return keys[size - 1]; }
    const uint64_t* begin() const { return keys; }
    const uint64_t* end() const { return keys + size; }
private:
    static const int kCapacity = 102;
    uint64_t keys[kCapacity];
    int size = 0;
};

struct ChessBoard {
    enum class CheckStatus { None, Check, Checkmate, Stalemate, Draw };
    enum class Turn { white, black } turn;
    enum class Piece : int8_t { king, queen, white_pawn, black_pawn, rook, bishop, knight }; // Defining chess piece types
    // Value of each piece (defined in eval.cpp), looked up as pieceValues[piece]
    struct PieceValues {
        int values[7];
        int operator[](Piece p) const { return values[static_cast<int>(p)]; }
    };
    static const PieceValues pieceValues;
    using PieceMap = SquareMap<Piece>;

    Pos LastMoveTo;
    Pos LastMoveFrom;
    Piece LastMovePiece = Piece::king; // Piece that made the last move (en passant needs to know it was a pawn)
    PieceMap white_pieces, black_pieces; // Positions of pieces for each player

    // Castling rights as bit flags (lost when the king or that rook moves, or the rook is captured)
    enum Castle { WhiteKingSide = 1, WhiteQueenSide = 2, BlackKingSide = 4, BlackQueenSide = 8 };
    int castlingRights = 0;

    // Draw tracking: half-moves since the last capture or pawn move (50-move rule), and the
    // position keys since then (repetition can't reach back past a capture or pawn move)
    int halfmoveClock = 0;
    PositionHistory history;

    PieceMap& moverPieces() { return turn == Turn::white ? white_pieces : black_pieces; } // Function to retrieve pieces of the current player
    PieceMap& opponentPieces() { return turn == Turn::white ? black_pieces : white_pieces; } // Function to retrieve pieces of the opponent player

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

    // A 64-bit number for the position (Zobrist hash), equal for equal positions,
    // so repetitions can be found by comparing numbers
    uint64_t positionKey() const;

    // Why the game is drawn by rule (50-move, threefold repetition, insufficient material), or nullptr
    const char* drawReason() const;

    // Check, checkmate, stalemate or draw for the side to move
    CheckStatus StatusCheck();
};
