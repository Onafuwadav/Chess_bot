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
    LastMove.clear();
    LastMoveTo = Pos();
    LastMoveFrom = Pos();

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

    // Castling rights (fields[2]) aren't tracked yet

    // En passant square: recreate the double pawn push that allows it
    if (fields.size() > 3 && fields[3] != "-") {
        Pos ep = fromAlgebraic(fields[3]);
        if (ep.x < 0 || (ep.y != 3 && ep.y != 6))
            return false;
        int dir = ep.y == 3 ? 1 : -1; // rank 3: white pawn went up, rank 6: black pawn went down
        LastMoveFrom = Pos(ep.x, ep.y - dir);
        LastMoveTo = Pos(ep.x, ep.y + dir);
        LastMove = opponentPieces();
    }
    return true;
}

// Function to execute move on board(checking first if move is possible and legal) then switching turn
//######(change to add piece square tables and such and to allow for user to choose what piece to promote the pawn to)#######
bool ChessBoard::makeMove(Pos from, Pos to) {
    std::vector<Pos> allowed = possibleMoves(*this, from, LastMoveTo, LastMoveFrom, LastMove[LastMoveTo]);
    if (find(allowed.begin(), allowed.end(), to) == allowed.end())
        return false;

    // check for en-passant
     if((LastMove[LastMoveTo] == Piece::white_pawn || LastMove[LastMoveTo] == Piece::black_pawn) && (moverPieces()[from] == Piece::white_pawn || moverPieces()[from] == Piece::black_pawn)
      && (!(white_pieces.count(to) || black_pieces.count(to))) && (LastMoveTo.x == from.x - 1 || LastMoveTo.x == from.x + 1) && (LastMoveTo.y == to.y)){
        if(turn == Turn::white)
            opponentPieces().erase(Pos(to.x, to.y-1));
        else
            opponentPieces().erase(Pos(to.x, to.y+1));
     }

    // save last move made
    LastMoveTo = to;
    LastMoveFrom = from;
    LastMove = moverPieces();

    opponentPieces().erase(to);
    moverPieces()[to] = moverPieces()[from];
    moverPieces().erase(from);
    // allow for user to choose what piece to promote the pawn to not just queen
    if ((moverPieces()[to] == Piece::white_pawn || moverPieces()[to] == Piece::black_pawn) && (to.y == 1 || to.y == 8))
        moverPieces()[to] = Piece::queen;
    flipTurn();
    return true;
}

bool ChessBoard::isKingUnderAttack(ChessBoard& board, Turn whichKing) {
    Piece kingPiece = Piece::king;
    Turn opponentTurn = whichKing;
    (void)opponentTurn;

    Pos kingPos;
    for (const auto& piece : board.opponentPieces()) {
        if (piece.second == kingPiece) {
            kingPos = piece.first;
            break;
        }
    }

    for (const auto& piece : board.moverPieces()) {
        std::vector<Pos> currentMoves = possibleMoves(*this, piece.first, LastMoveTo, LastMoveFrom, LastMove[LastMoveTo]);

        // Check if the square to check is among the current possible moves
        if (std::find(currentMoves.begin(), currentMoves.end(), kingPos) != currentMoves.end()) {
            return true;
        }
        else
        return false;
    }
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
    if ( isKingUnderAttack(board, turn) ) {
        bool canEscape = false; // Assuming the king can escape
        // Check if the king can move to a safe position
        for (const auto& move : possibleMoves(board, kingPos, board.LastMoveTo, board.LastMoveFrom, LastMove[LastMoveTo])) {
            (void)move;
            if (!isKingUnderAttack(board, turn)) {
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
