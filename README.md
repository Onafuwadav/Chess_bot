# Chess_bot

[![Build](https://github.com/Onafuwadav/Chess_bot/actions/workflows/build.yml/badge.svg)](https://github.com/Onafuwadav/Chess_bot/actions/workflows/build.yml)

## Quick start: play against it (Windows, nothing to build)

1. Download **[Chess_bot-windows.zip](https://github.com/Onafuwadav/Chess_bot/releases/latest/download/Chess_bot-windows.zip)**
   from the latest release and unzip it.
2. Install [ChessX](https://chessx.sourceforge.io/), then in Edit > Preferences > Engines add a UCI
   engine whose command is `chess_uci.exe` from the unzipped folder, and start a game against it.
3. Optional hand-drawn pieces and board: double-click `chessx-theme\install_chessx_theme.bat`,
   restart ChessX, and choose **chess-pack** for pieces and board in Edit > Preferences > Board.

Or double-click `chess_cli.exe` to play in the terminal. `chess_uci.exe` also works in any other
UCI GUI (Cute Chess, Arena, En Croissant). `HOW_TO_PLAY.txt` in the zip has the details.

## About
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
colours swapped, and estimates a rating and [testing/match/summary.ps1](testing/match/summary.ps1) reads every .pgn file in [testing/match/results/](testing/match/results/) and prints WLD against different Engines at different Time control's in easy to read format. 

#### `match.ps1` parameters
| Parameter | Default | What it does |
|---|---|---|
| `-Name` | `Chess_bot` | Name your engine is saved under in the results. Change it for each version so `summary.ps1` keeps results apart. |
| `-Elo` | `1320, 1500, 1700` | List of Stockfish `UCI_Elo` levels to play against, one match per level (Stockfish's range is 1320–3190). |
| `-Games` | `20` | Games per level. Each opening is played twice with colours swapped, so use an even number. More games give a smaller ± range. |
| `-TimeControl` | `10+0.1` | Clock for each side in seconds, `base+increment` (e.g. `120+1`), or `moves/seconds` (e.g. `40/60`). Use `120+1` for rating estimates. |
| `-Concurrency` | `2` | How many games run at the same time. Keep at ≈ half your CPU cores or engines get less thinking time. |
| `-CuteChess` | `%LOCALAPPDATA%\Programs\Cute Chess\cutechess-cli.exe` | Path to `cutechess-cli.exe`, if Cute Chess is installed elsewhere. |
| `-Stockfish` | first `stockfish*.exe` in `stockfish\` | Path to the Stockfish `.exe` to play against. |
| `-Engine` | `build\chess_uci.exe` | Path to your engine's `.exe`. Point at saved copy to re-test an older version. |


#### `summary.ps1` parameters
| Parameter | Default | What it does |
|---|---|---|
| `-TimeControl` | all | Only count games at this time control, e.g. `120+1`. |
| `-Engine` | all | Only show this engine's results, e.g. `Chess_bot_v2` (name given with `match.ps1 -Name`). |
| `-ResultsDir` | `testing\match\results` | Folder of `.pgn` files to read. |

Example
```powershell
.\testing\match\match.ps1 -Name **Engine name** -Elo 1400,1500,1600,1700 -Games 50 -TimeControl "120+1"

.\testing\match\summary.ps1 -TimeControl "120+1"
```
If PowerShell says running scripts is disabled, run
`Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass` in that window first.

#### How the numbers are calculated
All of these come from the standard Elo model: a player rated `D` points above their opponent is
expected to score `E = 1 / (1 + 10^(-D / 400))`, so +200 Elo expects about 76%, and 0 expects 50%.

- **Score:** points per game, where a win is 1, a draw is ½ and a loss is 0.
  e.g. 29 wins, 10 losses and 1 draw is 29.5 / 40 = 74%.
- **Elo diff:** the formula above turned around to find `D` from the score:
  `D = -400 x log10(1 / score - 1)`. A 74% score gives +179.
- **± range:** the 95% error margin. The results of the individual games (1, ½ or 0) vary around the
  average score; dividing that spread by √games gives the uncertainty in the score, and
  score ± 1.96 x uncertainty is converted to Elo with the same formula. More games give a smaller
  range. Lopsided results give a wide one, because near 0% or 100% a small change in score is a big
  change in Elo (and exactly 0% or 100% has no finite Elo at all).
- **Estimate:** the opponent's rating + Elo diff, e.g. 1500 + 179 = 1679.
- **Overall estimate:** one rating that fits the results against every level at once. More reliable than any single row because every game counts towards it.

#### Why 120+1, and how far to trust the result
Stockfish's documentation says its `UCI_Elo` setting "has been calibrated at a time control of
120s+1s and anchored to CCRL 40/4", so 120s per side plus 1s per move is where its labels are most
accurate.

Elo is relative: it only says how a player scores against the pool it was measured in. This one is
measured on the CCRL computer rating list's scale, so it is not a FIDE or chess.com rating. A
weakened Stockfish is also not a real 1500 player: it plays strong moves and then deliberately
picks worse ones at random, so it blunders differently from a human or a weak engine. That makes
this a useful, repeatable yardstick for comparing versions of Chess_bot, but an approximate absolute
rating.

#### Results: Chess_bot at 120+1 (120 games so far)
| Stockfish UCI_Elo | Games | W-L-D | Score | Elo diff | Estimate |
|---|---|---|---|---|---|
| 1400 | 50 | 46-4-0 | 92% | +424 ± 316 | 1824 |
| 1500 | 40 | 29-10-1 | 74% | +179 ± 130 | 1679 |
| 1600 | 17 (running) | 6-11-0 | 35% | -105 ± 196 | 1495 |
| 1700 | 13 (running) | 5-8-0 | 38% | -82 ± 226 | 1618 |

**Overall estimate: about ----** (200 games).

#### How the overall estimate is calculated
The rows above disagree (1824, 1679, 1495, 1618) because each has its own error. The overall
estimate finds the single rating `R` that would be expected to score exactly the points Chess_bot
actually scored against all of them:

`sum over every opponent of  games x 1 / (1 + 10^((opponent - R) / 400))  =  points scored`

The script finds `R` by bisection (try a rating, check whether the expected points are too high or
too low, halve the range, repeat). With the real results, `R = ----`:

| Opponent | Games | Expected score at 1670 | Expected points | Actual points |
|---|---|---|---|---|
| 1400 | 50 | 1 / (1 + 10^(-270/400)) = 0.826 | 41.3 | 46 |
| 1500 | 40 | 1 / (1 + 10^(-170/400)) = 0.727 | 29.1 | 29.5 |
| 1600 | 17 | 1 / (1 + 10^(-70/400)) = 0.599 | 10.2 | 6 |
| 1700 | 13 | 1 / (1 + 10^(30/400)) = 0.457 | 5.9 | 5 |
| **Total** | **120** | | **86.5** | **86.5** |


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
engine/                     the engine: board, move generation, evaluation, search, perft
play/cli/                   terminal game (chess_cli)
play/uci/                   UCI front-end for GUIs and matches (chess_uci)
play/gui-theme/             ChessX theme: install_chessx_theme.bat, make_chessx_theme.ps1
play/gui-theme/themes/      the theme converted for ChessX
play/gui-theme/chess-pack/  Joszs's original images (CC BY 4.0)
testing/perft/              perft tests
testing/rules/              draw rule tests
testing/search/             search and evaluation tests
testing/bench/              engine_bench (search speed)
testing/match/              Stockfish matches (match.ps1) and results summary (summary.ps1)
scripts/                    package.ps1: builds the release zip
.github/workflows/          GitHub Actions: build, test, and release on version tags
CMakeLists.txt              build for everything above
```

## Credits
- Chess art: [Chess Pieces and Board Pack](https://joszs.itch.io/chess-pack) by Joszs,
  CC BY 4.0 (see [play/gui-theme/CREDITS.md](play/gui-theme/CREDITS.md)).
- Piece-square tables: Tomasz Michniewski, *Simplified Evaluation Function*
  ([chessprogramming.org](https://www.chessprogramming.org/Simplified_Evaluation_Function)).
- Perft reference counts: [chessprogramming.org](https://www.chessprogramming.org/Perft_Results).
- Testing tools: [Catch2](https://github.com/catchorg/Catch2),
  [Stockfish](https://stockfishchess.org/), [Cute Chess](https://github.com/cutechess/cutechess).
