#include "movegen.h"
#include <algorithm>

using Piece = ChessBoard::Piece;

// Function to generate possible moves for a given piece
std::vector<Pos> possibleMoves(ChessBoard& board, const Pos& from, const Pos& LastTo, const Pos& Lastfrom, Piece lastpiece) {
    std::vector<Pos> moves; // Initialize vector to store possible moves

    // Lambdas to check ownership, opponent presence, board bounds, and free squares
    auto isOwn = [&](int dx, int dy) -> bool { return board.moverPieces().count(Pos(from, dx, dy)); };
    auto isOpponent = [&](int dx, int dy) -> bool { return board.opponentPieces().count(Pos(from, dx, dy)); };
    auto isInsideBoard = [&](int dx, int dy) -> bool { Pos p(from, dx, dy); return p.x < 9 && p.x > 0 && p.y < 9 && p.y > 0; };
    auto isFree = [&](int dx, int dy) -> bool { return !isOwn(dx, dy) && isInsideBoard(dx, dy) && !isOpponent(dx, dy); };

    // Lambda to add a move to the list of possible moves
    // (moves that put your own king in check are removed at the end)
    auto addMove = [&](int dx, int dy) -> bool {
        if (isFree(dx, dy) || isOpponent(dx, dy)) {
            moves.push_back(Pos(from, dx, dy));
            return true;
        }
        return false;
        };

    // If the selected position does not contain a piece, return an empty move list
    if (!isOwn(0, 0))
        return moves;

    // Get the type of piece at 'from' position
    auto moving_piece = board.moverPieces()[from];
    // Check possible moves based on the type of piece
    switch (moving_piece) {
    // Calculate moves for black pawn
    case Piece::black_pawn:
        if (isFree(0, -1)) addMove(0, -1);
        if (isFree(0, -1) && isFree(0, -2) && from.y == 7) addMove(0, -2);
        if (isOpponent(-1, -1)) addMove(-1, -1);
        if (isOpponent(1, -1)) addMove(1, -1);

        // En-passant for black
        if (LastTo.y == 4 && Lastfrom.y == 2 && from.y == 4 && lastpiece == Piece::white_pawn) {
            if (LastTo.x == from.x - 1 || LastTo.x == from.x + 1) {
                moves.push_back(Pos(LastTo.x, 3));
            }
        }
        break;

    // Calculate moves for white pawn
    case Piece::white_pawn:
        if (isFree(0, 1)) addMove(0, 1);
        if (isFree(0, 1) && isFree(0, 2) && from.y == 2) addMove(0, 2);
        if (isOpponent(-1, 1)) addMove(-1, 1);
        if (isOpponent(1, 1)) addMove(1, 1);

        // En-passant for white
        if (LastTo.y == 7 && Lastfrom.y == 5 && from.y == 5 && lastpiece == Piece::black_pawn) {
            if (LastTo.x == from.x - 1 || LastTo.x == from.x + 1) {
                moves.push_back(Pos(LastTo.x, 6));
            }
        }
        break;

    // Calculate moves for knight
    case Piece::knight:
        addMove(-2, -1); addMove(-2, 1); addMove(2, -1); addMove(2, 1);
        addMove(-1, -2); addMove(-1, 2); addMove(1, -2); addMove(1, 2);
        break;

    // Calculate moves for king
    case Piece::king:
        for (auto dy : { -1,0,1 })
            for (auto dx : { -1,0,1 })
                addMove(dy, dx);

        // Castling: king on its start square, right not lost, squares between empty,
        // and the king isn't in check and doesn't pass through an attacked square
        // (landing in check is caught by the legality filter below)
        {
            bool white = board.turn == ChessBoard::Turn::white;
            ChessBoard::Turn them = white ? ChessBoard::Turn::black : ChessBoard::Turn::white;
            int kingSideRight = white ? ChessBoard::WhiteKingSide : ChessBoard::BlackKingSide;
            int queenSideRight = white ? ChessBoard::WhiteQueenSide : ChessBoard::BlackQueenSide;
            if (from == Pos(4, white ? 1 : 8) && !board.isSquareAttacked(from, them)) {
                if ((board.castlingRights & kingSideRight) && isFree(-1, 0) && isFree(-2, 0)
                    && !board.isSquareAttacked(Pos(from, -1, 0), them))
                    moves.push_back(Pos(from, -2, 0));
                if ((board.castlingRights & queenSideRight) && isFree(1, 0) && isFree(2, 0) && isFree(3, 0)
                    && !board.isSquareAttacked(Pos(from, 1, 0), them))
                    moves.push_back(Pos(from, 2, 0));
            }
        }
        break;

    // Calculate moves for queen(rook + bishop)
    case Piece::queen:
    // Calculate moves for rook
    case Piece::rook:
        for (int n = 1; n < 9 && addMove(0, n) && !isOpponent(0, n); ++n);
        for (int n = 1; n < 9 && addMove(0, -n) && !isOpponent(0, -n); ++n);
        for (int n = 1; n < 9 && addMove(n, 0) && !isOpponent(n, 0); ++n);
        for (int n = 1; n < 9 && addMove(-n, 0) && !isOpponent(-n, 0); ++n);
        if (moving_piece != Piece::queen)
            break;
        [[fallthrough]];

    // Calculate moves for bishop
    case Piece::bishop:
        for (int n = 1; n < 9 && addMove(n, n) && !isOpponent(n, n); ++n);
        for (int n = 1; n < 9 && addMove(n, -n) && !isOpponent(n, -n); ++n);
        for (int n = 1; n < 9 && addMove(-n, n) && !isOpponent(-n, n); ++n);
        for (int n = 1; n < 9 && addMove(-n, -n) && !isOpponent(-n, -n); ++n);
        break;
    }

    // Remove moves that leave our own king in check: play each on a copy and look
    ChessBoard::Turn us = board.turn;
    moves.erase(std::remove_if(moves.begin(), moves.end(), [&](const Pos& to) {
        ChessBoard branch = board;
        branch.applyMove(from, to);
        return branch.isKingUnderAttack(us);
    }), moves.end());
    return moves;
}
