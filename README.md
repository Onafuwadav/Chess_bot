# Chess_bot

A chess engine written from scratch in C++. It searches with minimax and alpha-beta pruning,
orders moves so the pruning works well, and evaluates positions with material plus piece-square
tables. You can play it in a chess GUI such as ChessX or Cute Chess, and run
matches against Stockfish to measure its strength.

The long-term goal is a physical board: a camera reads the human's move and the engine replies.
That part is **not built yet** (see [Future work](#future-work-the-physical-board)).

## What it does

- **All the rules:** legal move generation including castling, en passant and promotion to any
  piece; check, checkmate and stalemate; draws by the 50-move rule, threefold repetition and
  insufficient material.
- **Verified move generation:** perft counts match the published numbers for six standard test
  positions (below).
- **Search:** minimax with alpha-beta pruning, MVV-LVA move ordering, quiescence search,
  iterative deepening with a time limit, and mate scores that prefer the quickest mate.
- **Evaluation:** material in centipawns plus piece-square tables, with the king's table blended
  between middlegame and endgame by game phase.
- **Ways to play:** a terminal game, and a UCI engine for chess GUIs (with a ChessX theme made from
  a hand-drawn piece set).

## How it works

### Board and move generation (`engine/board.*`, `engine/movegen.*`)

Each colour's pieces are kept in a 64-square array (`SquareMap`), with one bit per square saying
whether it is occupied. Squares are `Pos(x, y)`: `y` is the rank and `x = 1` is the h-file
(the board was first drawn from Black's side). Copying a board is a plain ~1 KB copy with no
memory allocation, so the search simply copies the board for each move it tries.

`possibleMoves` generates the moves of one piece, then removes any that would leave its own king
in check: it plays each move on a copy and asks `isSquareAttacked`, which looks outward from the
king square for pawns, knights, the enemy king and sliding pieces.

Repetitions are found with a 64-bit **Zobrist hash** of each position: the XOR of one random
number per (piece, colour, square), plus side to move, castling rights and en passant file. The
random numbers come from a mixing function (splitmix64) instead of a stored table, which saves
6 KB of memory for a future microcontroller build.

### Search (`engine/search.*`)

**Minimax** looks ahead `d` moves: White picks the move with the highest score, Black the lowest.
With about 35 legal moves per position, that is roughly 35^d positions.

**Alpha-beta pruning** gives the same answer with far fewer positions. `alpha` is the score White
is already guaranteed and `beta` the score Black is already guaranteed; once `alpha >= beta`, the
side choosing earlier would never allow this line, so the remaining moves are skipped. With
perfect move ordering this searches about 35^(d/2) positions instead of 35^d, roughly twice the
depth in the same time.

**Move ordering** is what makes that possible. Captures are tried first, ranked by MVV-LVA
(most valuable victim, least valuable attacker): `10 x victim - attacker`, so pawn-takes-queen
comes before queen-takes-pawn. Promotions come next, and at the root the best move from the
previous iteration goes first.

**Quiescence search** fixes the horizon effect. Instead of scoring a position the moment the
depth runs out (which would think Qxd5 won a pawn even if exd5 recaptures next move), the search
keeps playing captures until the position is quiet. The side to move may also "stand pat" and
take the static score, since it is never forced to capture.

**Iterative deepening** searches depth 1, then 2, 3, ... until the time runs out, and plays the
deepest finished result. The repeats are cheap (each depth costs about 1/35 of the next) and each
one tells the next which move to try first. Checkmate scores `1,000,000 - ply`, so a faster mate
scores higher.

### Evaluation (`engine/eval.*`)

`score = sum over white pieces (value + table[square]) - same for black`, in centipawns:
pawn 100, knight 320, bishop 330, rook 500, queen 900. The piece-square tables used come from Tomasz
Michniewski's *Simplified Evaluation Function*: knights prefer the centre, pawns gain value as
they advance, rooks like the 7th rank. The king uses two tables blended by game phase:

`phase = knights + bishops + 2 x rooks + 4 x queens` (24 at the start), and
`king bonus = (middlegame x phase + endgame x (24 - phase)) / 24`,

so the king shelters while the queens are on and walks to the centre in the endgame. Each table
is 64 bytes.

## Build

Needs CMake 3.16+ and a C++17 compiler. On Windows with MSYS2 (UCRT64 shell):

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
cmake -S . -B build -G Ninja
cmake --build build
```

The MinGW runtime is linked statically, so the `.exe` files run outside the MSYS2 shell.
Catch2 is downloaded automatically the first time CMake configures the tests.

| Program | What it is |
|---|---|
| `build/chess_cli.exe` | Play in the terminal |
| `build/chess_uci.exe` | UCI engine for chess GUIs and matches |
| `build/engine_tests.exe` | Perft, rules, evaluation and search tests |
| `build/engine_bench.exe [depth]` | Search speed on four positions |

## 1. Engine testing (no board)

### Tests

```bash
./build/engine_tests.exe            # about 1 s
./build/engine_tests.exe "[slow]"   # deep perft counts, about 11 s
```

### Perft

Perft counts every position reachable in exactly N moves. If any rule is wrong, the count is
wrong, so matching the published counts
([chessprogramming.org](https://www.chessprogramming.org/Perft_Results)) is strong evidence the
move generator is correct. All of these match:

| Position | Depth | Positions |
|---|---|---|
| Start position | 5 | 4,865,609 |
| Kiwipete (castling, en passant, pins) | 4 | 4,085,603 |
| Position 3 (en passant pins, endgame) | 5 | 674,624 |
| Position 4 (promotions, checks) | 4 | 422,333 |
| Position 5 | 4 | 2,103,487 |
| Position 6 | 4 | 3,894,594 |

### Speed

`engine_bench 4` searches four positions (start, Kiwipete, a middle game, a rook endgame) to
depth 4. How each change affected it:

| Version | Positions searched | Time |
|---|---|---|
| Plain minimax | 6,088,286 | 33.8 s |
| + alpha-beta pruning | 266,203 | 1.1 s |
| + piece-square tables | 365,470 | 1.5 s |
| + MVV-LVA move ordering | 47,372 | 0.65 s |
| + quiescence search | 146,585 | 2.2 s |
| + array board (replacing `std::map`) | 146,585 | 0.61 s |

Quiescence searches more positions because it plays out captures past the depth limit; that is
the cost of not blundering pieces at the horizon. The tables made pruning slightly less
effective at first because fewer moves tie, which move ordering then more than recovered.

Currently about **250,000 positions per second** on one core; depth 5 takes about 2.6 s for all
four positions. With 2 seconds per move it usually completes depth 5 in the middlegame.

### Strength against Stockfish

[testing/match/match.ps1](testing/match/match.ps1) plays Chess_bot against Stockfish limited to a
given rating (`UCI_LimitStrength`, `UCI_Elo`) using cutechess-cli, from a 16-opening book with
colours swapped, and estimates a rating from the score:
`Elo difference = -400 x log10(1 / score - 1)`.

```powershell
.\testing\match\match.ps1 -Elo ...,... -Games ... -TimeControl "..."
```
Testing against 1320, 1500, 1700:
First results, 10 games per level at 5s + 0.05s per game:

| Stockfish UCI_Elo | Chess_bot W-L-D |
|---|---|
| 1320 | 9-1-0 |
| 1500 | 3-7-0 |
| 1700 | 3-7-0 |
| 1900 | 0-10-0 |

That suggests roughly **1400-1600** ELO at this fast time control. Ten games per level gives error
bars of a few hundred Elo, and Stockfish's rating scale is calibrated at a different time control,
so treat this as a rough first estimate.

Testing against 1400, 1500, 1600, 1700:
Proper test results, 50 games per level at 120s + 1s per game:
| Stockfish UCI_Elo | Chess_bot W-L-D |
|---|---|
| 1320 | x-x-x |
| 1500 | x-x-x |
| 1700 | x-x-x |

That suggests roughly **xxxx-xxxx** ELO at this time control. Fifty games per level(took around 3hrs) gives a pretty accurate Elo rating, and this is the time control at which Stockfish's rating scale is calibrated at making it a more accurate estimate.

Needs [Cute Chess](https://github.com/cutechess/cutechess) (for cutechess-cli) and a
[Stockfish](https://stockfishchess.org/download/) `.exe` unzipped into `stockfish/` (not committed).

## 2. Human vs AI

### In a chess GUI
`chess_uci.exe` works in any UCI GUI.

The repo includes a hand drawn chess board and piece theme made by Joszs
[Chess Pieces and Board Pack](https://joszs.itch.io/chess-pack) (CC BY 4.0).

### In the terminal
`chess_cli.exe` draws the board in text and plays at 2 seconds per move. Moves are typed as four
digits `xyxy` (from-square then to-square, in the board's own coordinates shown around the
board), with an optional promotion letter, e.g. `2728n`. `h` shows help.

## 3. Future work: the physical board

The engine core (`engine/`) was written with a microcontroller in mind: no `iostream`, fixed-size
boards and tables, no memory allocation when a board is copied. Not yet done:

- **Microcontroller build.** A board copy is about 1 KB (most of it the repetition history), so a
  search about 15 moves deep needs roughly 15-20 KB. That fits an ESP32 or RP2040 (260-520 KB RAM)
  but not an Arduino Uno (2 KB). Still to do: replace the `std::vector` move lists with fixed
  arrays, then write a small sketch that reads a move over serial and replies with the engine's.
- **Computer vision.** A camera above the board, most likely processed on a Raspberry Pi or laptop
  rather than the microcontroller, finds which squares changed after the human's move and sends
  that move to the engine.
- **Showing the engine's move.** Either a speaker announcing it ("knight to f3"), LEDs under the
  squares, or eventually a robot arm that moves the piece itself.

Engine ideas: a transposition table (cache of searched positions keyed by the Zobrist hash),
killer moves, and an opening book.

## Project layout

```
engine/          the engine: board, move generation, evaluation, search, perft
play/cli/        terminal game
play/uci/        UCI front-end for GUIs and matches
play/gui-theme/  ChessX theme from Joszs's chess pack
testing/perft/   perft tests          testing/rules/   draw rule tests
testing/search/  search and eval tests
testing/bench/   engine_bench         testing/match/   Stockfish matches
```

## Credits

- Chess art: [Chess Pieces and Board Pack](https://joszs.itch.io/chess-pack) by Joszs,
  CC BY 4.0 (see [play/gui-theme/CREDITS.md](play/gui-theme/CREDITS.md)).
- Piece-square tables: Tomasz Michniewski, *Simplified Evaluation Function*
  ([chessprogramming.org](https://www.chessprogramming.org/Simplified_Evaluation_Function)).
- Perft reference counts: [chessprogramming.org](https://www.chessprogramming.org/Perft_Results).
- Testing tools: [Catch2](https://github.com/catchorg/Catch2),
  [Stockfish](https://stockfishchess.org/), [Cute Chess](https://github.com/cutechess/cutechess).
