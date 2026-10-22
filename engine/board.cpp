#include "board.h"
#include "movegen.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <vector>

// Function to reset the chessboard to initial state
void ChessBoard::reset() {
    turn = Turn::white;
    white_pieces.clear();
    black_pieces.clear();
    for (int i = 1; i < 9; ++i) {
        black_pieces[Pos(i, 7)] = Piece::black_pawn;
        white_pieces[Pos(i, 2)] = Piece::white_pawn;
    }
    int n = 1;
    for (auto piece : { Piece::rook,Piece::knight,Piece::bishop,Piece::king }) {
        white_pieces[Pos(n, 1)] = white_pieces[Pos(9 - n, 1)] = black_pieces[Pos(n, 8)] = black_pieces[Pos(9 - n, 8)] = piece;
        ++n;
    }
    black_pieces[Pos(5, 8)] = white_pieces[Pos(5, 1)] = Piece::queen;
    castlingRights = WhiteKingSide | WhiteQueenSide | BlackKingSide | BlackQueenSide;
    LastMoveTo = Pos();
    LastMoveFrom = Pos();
    LastMovePiece = Piece::king;
}

Pos ChessBoard::fromAlgebraic(const std::string& square) {
    if (square.size() != 2 || square[0] < 'a' || square[0] > 'h' || square[1] < '1' || square[1] > '8')
        return Pos();
    return Pos('h' - square[0] + 1, square[1] - '0');
}

std::string ChessBoard::toAlgebraic(const Pos& p) {
    return std::string{ char('h' - p.x + 1), char('0' + p.y) };
}

bool ChessBoard::loadFEN(const std::string& fen) {
    // Split into fields: placement, side to move, castling, en passant, (clocks are ignored)
    std::vector<std::string> fields;
    std::string field;
    for (char c : fen + ' ') {
        if (c != ' ') field += c;
        else if (!field.empty()) { fields.push_back(field); field.clear(); }
    }
    if (fields.size() < 2)
        return false;

    white_pieces.clear();
    black_pieces.clear();
    LastMoveTo = Pos();
    LastMoveFrom = Pos();
    LastMovePiece = Piece::king;

    // Placement goes rank 8 to rank 1, each rank from the a-file to the h-file
    int y = 8, file = 0;
    for (char c : fields[0]) {
        if (c == '/') { --y; file = 0; continue; }
        if (c >= '1' && c <= '8') { file += c - '0'; continue; }
        bool white = std::isupper(static_cast<unsigned char>(c));
        Piece piece;
        switch (std::tolower(static_cast<unsigned char>(c))) {
        case 'p': piece = white ? Piece::white_pawn : Piece::black_pawn; break;
        case 'n': piece = Piece::knight; break;
        case 'b': piece = Piece::bishop; break;
        case 'r': piece = Piece::rook; break;
        case 'q': piece = Piece::queen; break;
        case 'k': piece = Piece::king; break;
        default: return false;
        }
        if (file > 7 || y < 1)
            return false;
        (white ? white_pieces : black_pieces)[Pos(8 - file, y)] = piece;
        ++file;
    }

    turn = fields[1] == "b" ? Turn::black : Turn::white;

    castlingRights = 0;
    if (fields.size() > 2) {
        for (char c : fields[2]) {
            if (c == 'K') castlingRights |= WhiteKingSide;
            if (c == 'Q') castlingRights |= WhiteQueenSide;
            if (c == 'k') castlingRights |= BlackKingSide;
            if (c == 'q') castlingRights |= BlackQueenSide;
        }
    }

    // En passant square: recreate the double pawn push that allows it
    if (fields.size() > 3 && fields[3] != "-") {
        Pos ep = fromAlgebraic(fields[3]);
        if (ep.x < 0 || (ep.y != 3 && ep.y != 6))
            return false;
        int dir = ep.y == 3 ? 1 : -1; // rank 3: white pawn went up, rank 6: black pawn went down
        LastMoveFrom = Pos(ep.x, ep.y - dir);
        LastMoveTo = Pos(ep.x, ep.y + dir);
        LastMovePiece = ep.y == 3 ? Piece::white_pawn : Piece::black_pawn;
    }
    return true;
}

// Function to execute move on board(checking first if move is possible and legal) then switching turn
//######(change to add piece square tables and such)#######
bool ChessBoard::makeMove(Pos from, Pos to, Piece promotion) {
    std::vector<Pos> allowed = possibleMoves(*this, from, LastMoveTo, LastMoveFrom, LastMovePiece);
    if (find(allowed.begin(), allowed.end(), to) == allowed.end())
        return false;
    if (promotion != Piece::queen && promotion != Piece::rook && promotion != Piece::bishop && promotion != Piece::knight)
        return false;

    applyMove(from, to, promotion);
    return true;
}

void ChessBoard::applyMove(Pos from, Pos to, Piece promotion) {
    // check for en-passant: a pawn moving diagonally onto an empty square can only be en passant,
    // and the pawn it captures is beside it (same rank it started on, file it moved to)
    if ((moverPieces()[from] == Piece::white_pawn || moverPieces()[from] == Piece::black_pawn)
        && to.x != from.x && !(white_pieces.count(to) || black_pieces.count(to)))
        opponentPieces().erase(Pos(to.x, from.y));

    // Castling: the king moves two squares, so bring the rook across to the other side of it
    // (x = 1 is the h-file, so king side is towards x = 1)
    if (moverPieces()[from] == Piece::king && (to.x - from.x == 2 || to.x - from.x == -2)) {
        bool kingSide = to.x < from.x;
        Pos rookFrom(kingSide ? 1 : 8, from.y);
        Pos rookTo(kingSide ? 3 : 5, from.y);
        moverPieces().erase(rookFrom);
        moverPieces()[rookTo] = Piece::rook;
    }

    // A king move loses both rights; moving from or capturing on a corner loses that rook's right
    if (moverPieces()[from] == Piece::king)
        castlingRights &= turn == Turn::white ? ~(WhiteKingSide | WhiteQueenSide) : ~(BlackKingSide | BlackQueenSide);
    for (const Pos& p : { from, to }) {
        if (p == Pos(1, 1)) castlingRights &= ~WhiteKingSide;
        if (p == Pos(8, 1)) castlingRights &= ~WhiteQueenSide;
        if (p == Pos(1, 8)) castlingRights &= ~BlackKingSide;
        if (p == Pos(8, 8)) castlingRights &= ~BlackQueenSide;
    }

    // save last move made
    LastMoveTo = to;
    LastMoveFrom = from;
    LastMovePiece = moverPieces()[from];

    opponentPieces().erase(to);
    moverPieces()[to] = moverPieces()[from];
    moverPieces().erase(from);
    // Promotion: the pawn becomes the chosen piece
    if ((moverPieces()[to] == Piece::white_pawn || moverPieces()[to] == Piece::black_pawn) && (to.y == 1 || to.y == 8))
        moverPieces()[to] = promotion;
    flipTurn();
}

// Looks outward from the square for each kind of attacker, instead of
// generating every enemy move: about 50 lookups at most.
bool ChessBoard::isSquareAttacked(const Pos& square, Turn by) const {
    const std::map<Pos, Piece>& attackers = by == Turn::white ? white_pieces : black_pieces;
    auto attackerAt = [&](int dx, int dy, Piece piece) -> bool {
        auto it = attackers.find(Pos(square, dx, dy));
        return it != attackers.end() && it->second == piece;
    };
    auto isEmpty = [&](int dx, int dy) -> bool {
        Pos p(square, dx, dy);
        return !white_pieces.count(p) && !black_pieces.count(p);
    };
    auto isInsideBoard = [&](int dx, int dy) -> bool { Pos p(square, dx, dy); return p.x < 9 && p.x > 0 && p.y < 9 && p.y > 0; };

    // Pawns attack diagonally forward, so look one rank behind the square
    if (by == Turn::white) {
        if (attackerAt(-1, -1, Piece::white_pawn) || attackerAt(1, -1, Piece::white_pawn)) return true;
    } else {
        if (attackerAt(-1, 1, Piece::black_pawn) || attackerAt(1, 1, Piece::black_pawn)) return true;
    }

    static const int knightJumps[8][2] = { {-2,-1},{-2,1},{2,-1},{2,1},{-1,-2},{-1,2},{1,-2},{1,2} };
    for (auto& j : knightJumps)
        if (attackerAt(j[0], j[1], Piece::knight)) return true;

    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
            if ((dx || dy) && attackerAt(dx, dy, Piece::king)) return true;

    // Sliding pieces: walk each ray to the first piece and see what it is
    static const int rays[8][2] = { {0,1},{0,-1},{1,0},{-1,0},{1,1},{1,-1},{-1,1},{-1,-1} };
    for (int r = 0; r < 8; ++r) {
        Piece slider = r < 4 ? Piece::rook : Piece::bishop;
        int dx = rays[r][0], dy = rays[r][1];
        int n = 1;
        while (isInsideBoard(dx * n, dy * n) && isEmpty(dx * n, dy * n))
            ++n;
        if (attackerAt(dx * n, dy * n, slider) || attackerAt(dx * n, dy * n, Piece::queen))
            return true;
    }
    return false;
}

bool ChessBoard::isKingUnderAttack(Turn whichKing) const {
    const std::map<Pos, Piece>& own = whichKing == Turn::white ? white_pieces : black_pieces;
    for (const auto& piece : own)
        if (piece.second == Piece::king)
            return isSquareAttacked(piece.first, whichKing == Turn::white ? Turn::black : Turn::white);
    return false;
}

ChessBoard::CheckStatus ChessBoard::StatusCheck(ChessBoard& board) {
    Piece kingPiece = Piece::king;
    Pos kingPos;
    for (const auto& piece : board.opponentPieces()) {
        if (piece.second == kingPiece) {
            kingPos = piece.first;
            break;
        }
    }

    // check for checkmate
    if ( isKingUnderAttack(turn) ) {
        bool canEscape = false; // Assuming the king can escape
        // Check if the king can move to a safe position
        for (const auto& move : possibleMoves(board, kingPos, board.LastMoveTo, board.LastMoveFrom, LastMovePiece)) {
            (void)move;
            if (!isKingUnderAttack(turn)) {
                canEscape = true; // King can escape
                break;
            }
        }

        if (!canEscape) {
            // King is under checkmate
            std::cout << "Checkmate!" << std::endl;
            return CheckStatus::Checkmate;
        } else {
            // King is under check but not checkmate
            std::cout << "King is in Check!" << std::endl;
            return CheckStatus::Check;
        }
    }
    else
    return CheckStatus::None;
}

// add check/checkmate function
bool ChessBoard::hasKing() {
    for (auto& p : moverPieces())
        if (p.second == Piece::king)
            return true;
    return false;
}
