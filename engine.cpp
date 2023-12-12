#include<iostream>
#include<string>
#include<vector>
#include<map>
#include<algorithm>
#define quote(x) #x

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
    static std::map<Piece, int> pieceValues; // Map to hold values for each piece and their values
    bool show_coordinates = false;
    
    Pos LastMoveTo;
    Pos LastMoveFrom;
    std::map<Pos, Piece> LastMove; // Saving last move made
    std::map<Pos, Piece> white_pieces, black_pieces; // Maps to store positions of pieces for each player
    std::map<Pos, Piece>& moverPieces() { return turn == Turn::white ? white_pieces : black_pieces; } // Function to retrieve pieces of the current player
    std::map<Pos, Piece>& opponentPieces() { return turn == Turn::white ? black_pieces : white_pieces; } // Function to retrieve pieces of the opponent player

    // Function to reset the chessboard to initial state
    void reset() {
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

    // Switches turn between black and white player after turn end
    void flipTurn() { turn = turn == Turn::white ? Turn::black : Turn::white; }

    // Function to execute move on board(checking first if move is possible and legal) then switching turn 
    //######(change to add piece square tables and such and to allow for user to choose what piece to promote the pawn to)#######
    bool makeMove(Pos from, Pos to) {
        std::vector<Pos> allowed = possibleMoves(from, LastMoveTo, LastMoveFrom, LastMove[LastMoveTo]);
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

    // Function to generate possible moves for a given piece
    std::vector<Pos> possibleMoves(const Pos& from, const Pos& LastTo, const Pos& Lastfrom, Piece lastpiece) {
        std::vector<Pos> moves; // Initialize vector to store possible moves

        // Lambdas to check ownership, opponent presence, board bounds, and free squares
        auto isOwn = [&](int dx, int dy) -> bool { return moverPieces().count(Pos(from, dx, dy)); };
        auto isOpponent = [&](int dx, int dy) -> bool { return opponentPieces().count(Pos(from, dx, dy)); };
        auto isInsideBoard = [&](int dx, int dy) -> bool { Pos p(from, dx, dy); return p.x < 9 && p.x > 0 && p.y < 9 && p.y > 0; };
        auto isFree = [&](int dx, int dy) -> bool { return !isOwn(dx, dy) && isInsideBoard(dx, dy) && !isOpponent(dx, dy); };

        // Lambda to add a move to the list of possible moves
        auto addMove = [&](int dx, int dy) -> bool {
            // change this so you cant do a move that puts you in check






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
        auto moving_piece = moverPieces()[from];
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

        // Calculate moves for bishop
        case Piece::bishop:
            for (int n = 1; n < 9 && addMove(n, n) && !isOpponent(n, n); ++n);
            for (int n = 1; n < 9 && addMove(n, -n) && !isOpponent(n, -n); ++n);
            for (int n = 1; n < 9 && addMove(-n, n) && !isOpponent(-n, n); ++n);
            for (int n = 1; n < 9 && addMove(-n, -n) && !isOpponent(-n, -n); ++n);
            break;
        }
        return moves;
    }

    bool isKingUnderAttack(ChessBoard& board, Turn whichKing) {
        Piece kingPiece = Piece::king;
        Turn opponentTurn = whichKing;

        Pos kingPos;
        for (const auto& piece : board.opponentPieces()) {
            if (piece.second == kingPiece) {
                kingPos = piece.first;
                break;
            }
        }

        for (const auto& piece : board.moverPieces()) {
            std::vector<Pos> currentMoves = possibleMoves(piece.first, LastMoveTo, LastMoveFrom, LastMove[LastMoveTo]);

            // Check if the square to check is among the current possible moves
            if (std::find(currentMoves.begin(), currentMoves.end(), kingPos) != currentMoves.end()) {
                return true;
            } 
            else
            return false;
        }
    }

    CheckStatus StatusCheck(ChessBoard& board) {        
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
            for (const auto& move : board.possibleMoves(kingPos, board.LastMoveTo, board.LastMoveFrom, LastMove[LastMoveTo])) {
                if (!isKingUnderAttack(board, turn)) {
                    canEscape = true; // King can escape
                    break;
                }
            }

            if (!canEscape) {
                // King is under checkmate
                return CheckStatus::Checkmate;
                std::cout << "Checkmate!" << std::endl;
            } else {
                // King is under check but not checkmate
                return CheckStatus::Check;
                std::cout << "King is in Check!" << std::endl;
            }
        }
        else
        return CheckStatus::None;
    }  

    void printBoard() {
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
                if (white_pieces.count(Pos(x, y)))
                    std::cout << sprites[white_pieces[Pos(x, y)]];
                else if (black_pieces.count(Pos(x, y)))
                    std::cout << (char)tolower(sprites[black_pieces[Pos(x, y)]]);
                else
                    std::cout << " ";
                std::cout << "  ";
            }
            std::cout << "|  " << y << std::endl << "     |_____|_____|_____|_____|_____|_____|_____|_____|";
        }
        std::cout << std::endl << std::endl << "        1     2     3     4     5     6     7     8   " << std::endl << std::endl;
        int result = score();
        std::cout << "The result is: " << result << std::endl << std::endl;
    }

    void printHelp() {
        std::cout << std::endl << "* h: help, q: quit, p: show board, c: toggle show coordinates inside squares" << std::endl << "* Input format: yxyx is from-to coordinates, e.g: 1715 moves (x,y)=(1,7) to (x,y)=(1,5)" << std::endl << std::endl;
    }

    /* False to exit */
    bool promptInput() {
        std::string move;
    illegalmove:
        if (turn == Turn::white){
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
            printBoard();
            move = "";
            goto illegalmove;
        }

        if (move == "p") {
            printBoard();
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
        if (!makeMove(from, to)) {
            std::cout << "* Illegal move" << std::endl;
            move = "";
            goto illegalmove;
        }
        printBoard();
        return true;
    }

    // make different score function for ai to add psf/sfe 
    int score() {
        int sumWhite = 0;
        for (auto& p : white_pieces)
            sumWhite += pieceValues[p.second];
        int sumBlack = 0;
        for (auto& p : black_pieces)
            sumBlack += pieceValues[p.second];
        return sumWhite - sumBlack;
    }

    // add check/checkmate function
    bool hasKing() {
        for (auto& p : moverPieces())
            if (p.second == Piece::king)
                return true;
        return false;
    }

    struct Move {
        Pos from, to;
        int score;
    };

    Move minimax(int depth, bool minimize) {
        Move best_move;
        best_move.score = -1000000 + 2000000 * minimize;
        if (0 == depth) {
            best_move.score = score();
            return best_move;
        }

        for (auto& from : moverPieces()) {
            for (auto& to : possibleMoves(from.first, LastMoveTo, LastMoveFrom, LastMove[LastMoveTo])) {
                ChessBoard branch = *this;
                branch.makeMove(from.first, to);
                Move option = branch.minimax(depth - 1, !minimize);
                if ((option.score > best_move.score && !minimize) || (option.score < best_move.score && minimize)) {
                    best_move.score = option.score;
                    best_move.from = from.first;
                    best_move.to = to;
                }
            }
        }
        return best_move;
    }

    void AIMove() {
        bool minimize = turn == Turn::white ? true : false;
        Move m = minimax(4, minimize);
        makeMove(m.from, m.to);
        printBoard();
    }
};

std::map<ChessBoard::Piece, int> ChessBoard::pieceValues{ {ChessBoard::Piece::king, 10000},
{ChessBoard::Piece::queen, 9}, {ChessBoard::Piece::black_pawn, 1}, {ChessBoard::Piece::white_pawn, 1},
{ChessBoard::Piece::bishop, 3},{ChessBoard::Piece::knight, 3},{ChessBoard::Piece::rook, 5}, };

main() {
    ChessBoard game;
    std::cout << std::endl << "* Chesscomputer v.0.1" << std::endl;
    game.printHelp();
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
            game.AIMove();
        else
            game.printBoard();

        while (gameon = game.promptInput()) {
            if (!game.hasKing()) {
                std::cout << "* You are victorious!" << std::endl;
                break;
            }
            game.AIMove();
            if (!game.hasKing()) {
                std::cout << "* You are defeated!" << std::endl;
                break;
            }
        }
    }
    return 0;
}