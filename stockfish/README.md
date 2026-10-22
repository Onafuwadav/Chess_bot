# Stockfish goes here

`testing/match/match.ps1` plays Chess_bot against Stockfish to estimate its rating. Stockfish
isn't included in this repo, so to run
matches:

1. Download Stockfish for Windows from https://stockfishchess.org/download/
2. Unzip it and put the `.exe` in this folder. Any file named `stockfish*.exe` is found.
   - Or leave it anywhere and pass its path: `match.ps1 -Stockfish "C:\path\to\stockfish.exe"`.

The match script weakens Stockfish to a set rating with its `UCI_LimitStrength` and `UCI_Elo`
options, so it needs Stockfish specifically.
Anything you put in this folder stays on your computer: it's in `.gitignore`.

Stockfish is free software under the GPL v3: https://github.com/official-stockfish/Stockfish
