# Match results

`match.ps1` saves every game here as PGN, one file per Stockfish level
(`vs_stockfish_<elo>.pgn`). New games are added to the end of the file, so results from
several runs build up.

The game files aren't committed (they're in `.gitignore`), so this folder is empty on GitHub
apart from this note. Run a match to fill it, then:

```powershell
.\testing\match\summary.ps1 -TimeControl "120+1"
```

The games can also be opened in ChessX to replay them.
