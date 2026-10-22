<#
.SYNOPSIS
  Plays Chess_bot against Stockfish at one or more strength levels and estimates its rating.

.EXAMPLE
  .\testing\match\match.ps1                          # 20 games each vs Stockfish 1320, 1500, 1700
  .\testing\match\match.ps1 -Elo 1320 -Games 50      # more games = smaller error bars
  .\testing\match\match.ps1 -TimeControl "120+1"     # the time control UCI_Elo is calibrated at
  .\testing\match\match.ps1 -Name Chess_bot_v2       # label a new engine version in the results
  .\testing\match\summary.ps1                        # W-L-D of every engine vs every opponent

  Needs build\chess_uci.exe (cmake --build build), a Stockfish .exe in .\stockfish\,
  and cutechess-cli (comes with Cute Chess). Games are saved to testing\match\results\.
#>
param(
    [string]$Name = "Chess_bot",
    [int[]]$Elo = @(1320, 1500, 1700),
    [int]$Games = 20,
    [string]$TimeControl = "10+0.1",
    [int]$Concurrency = 2,
    [string]$CuteChess = "$env:LOCALAPPDATA\Programs\Cute Chess\cutechess-cli.exe",
    [string]$Stockfish = "",
    [string]$Engine = ""
)

$root = (Resolve-Path "$PSScriptRoot\..\..").Path
if (-not $Engine) { $Engine = Join-Path $root "build\chess_uci.exe" }
if (-not $Stockfish) {
    $Stockfish = Get-ChildItem (Join-Path $root "stockfish") -Filter "stockfish*.exe" -ErrorAction SilentlyContinue |
        Select-Object -First 1 -ExpandProperty FullName
}
foreach ($file in @($Engine, $Stockfish, $CuteChess)) {
    if (-not $file -or -not (Test-Path $file)) { Write-Error "Not found: '$file' (see the help at the top of this script)"; exit 1 }
}

$results = Join-Path $PSScriptRoot "results"
New-Item -ItemType Directory -Force $results | Out-Null
$summary = @()

# cutechess describes each result like "White wins by adjudication" or "Black mates".
# Split that into how it ended ("Adjudication", "Mate") and who won ("White wins").
function Split-Result([string]$detail) {
    if ($detail -match '^(White|Black) mates$') { return "Mate", "$($Matches[1]) wins" }
    if ($detail -match '^(White|Black) wins on time$') { return "Time", "$($Matches[1]) wins" }
    if ($detail -match '^(White|Black) wins by (.+)$') {
        return ($Matches[2].Substring(0, 1).ToUpper() + $Matches[2].Substring(1)), "$($Matches[1]) wins"
    }
    if ($detail -match '^Draw by (.+)$') { return $Matches[1], "" }
    return $detail, ""
}

# Prints our engine's results grouped as Draw by / Loss by / Win by, e.g.
#   Loss by:
#      Mate: Black wins: 3, White wins: 8
function Write-ResultBreakdown([string]$player, $entries) {
    Write-Host "Player: $player"
    foreach ($outcome in "Draw", "Loss", "Win") {
        $mine = @($entries | Where-Object { $_.Outcome -eq $outcome })
        if (-not $mine) { continue }
        Write-Host "   $outcome by:"
        foreach ($method in ($mine | Group-Object Method)) {
            $parts = $method.Group | ForEach-Object { if ($_.Who) { "$($_.Who): $($_.Count)" } else { "$($_.Count)" } }
            Write-Host ("      {0}: {1}" -f $method.Name, ($parts -join ", "))
        }
    }
}

foreach ($level in $Elo) {
    Write-Host "`n=== $Name vs Stockfish (UCI_Elo $level): $Games games, $TimeControl ===" -ForegroundColor Cyan
    $pgn = Join-Path $results "vs_stockfish_$level.pgn"
    $scoreLine = ""; $eloLine = ""
    $separatorShown = $false
    $player = ""; $breakdown = @()   # the end-of-match "Player:" block for our engine

    # -repeat plays each opening twice with colours swapped; draw/resign rules shorten decided games
    & $CuteChess `
        -engine "name=$Name" "cmd=$Engine" `
        -engine "name=Stockfish_$level" "cmd=$Stockfish" option.UCI_LimitStrength=true "option.UCI_Elo=$level" option.Threads=1 `
        -each proto=uci "tc=$TimeControl" `
        -games $Games -repeat -recover -concurrency $Concurrency `
        -openings "file=$PSScriptRoot\openings.pgn" format=pgn order=random `
        -draw movenumber=40 movecount=8 score=10 `
        -resign movecount=4 score=800 `
        -pgnout $pgn 2>&1 | ForEach-Object {
            $line = "$_"
            if ($line -match "^Score of") { $scoreLine = $line }

            if ($line -match '^\.\.\.\s+(.*)$') {
                # End-of-match totals ("...      Chess_bot playing White: ..."): separate them from the games
                if (-not $separatorShown) { Write-Host ("-" * 52); $separatorShown = $true }
                # "White vs Black" counts results by colour regardless of engine, so it's left out
                if ($Matches[1] -notmatch '^White vs Black') { Write-Host $Matches[1] }
            } elseif ($line -match "^Elo difference") {
                # "Elo difference: x +/- y, LOS: z %, DrawRatio: w %" -> one figure per line
                $eloLine = $line
                $line -split ",\s*" | ForEach-Object { Write-Host ($_ -replace " %", "%") }
            } elseif ($line -match "^SPRT:") {
                # SPRT (early stopping for version-vs-version tests) isn't used, so its line is always empty
            } elseif ($line -match '^Player: (.+)$') {
                $player = $Matches[1]
            } elseif ($player -and $line -match '^\s+"(?:(Win|Loss|Draw): )?(.+)":\s*(\d+)$') {
                # e.g.   "Loss: Black mates": 3   (only kept for our engine, not Stockfish)
                if ($player -eq $Name) {
                    $outcome = if ($Matches[1]) { $Matches[1] } else { "Draw" }
                    $count = [int]$Matches[3]
                    $method, $who = Split-Result $Matches[2]
                    $breakdown += [pscustomobject]@{ Outcome = $outcome; Method = $method; Who = $who; Count = $count }
                }
            } elseif ($line -match "^Finished match") {
                Write-ResultBreakdown $Name $breakdown
            } else {
                Write-Host $line
            }
        }

    # "Score of A vs B: 16 - 23 - 1  [0.412] 40" -> wins, losses, draws, score fraction, games
    $played = "?"; $wld = "?"; $score = "?"
    if ($scoreLine -match ":\s*(\d+)\s*-\s*(\d+)\s*-\s*(\d+)\s*\[([\d.]+)\]\s*(\d+)") {
        $wld = "$($Matches[1])-$($Matches[2])-$($Matches[3])"
        $score = "{0:P0}" -f [double]$Matches[4]
        $played = $Matches[5]
    }

    # "Elo difference: -61.4 +/- 111.8, LOS: ..." -> estimated rating = level + difference
    $diff = "?"; $estimate = "?"
    if ($eloLine -match "Elo difference:\s*(-?[\d.]+|-?inf)\s*\+/-\s*([\d.]+|nan)") {
        if ($Matches[1] -notmatch "inf") {
            # cutechess gives "nan" for the range when there are too few games to work it out
            $range = if ($Matches[2] -eq "nan") { "?" } else { "{0:0}" -f [double]$Matches[2] }
            $diff = "{0:+0;-0} (+/- {1})" -f [double]$Matches[1], $range
            $estimate = "{0:0}" -f ($level + [double]$Matches[1])
        } else {
            $diff = "n/a (won or lost all)"; $estimate = ""
        }
    }
    $summary += [pscustomobject]@{
        "Stockfish Elo" = $level; Games = $played; "W-L-D" = $wld; Score = $score
        "Elo diff" = $diff; "$Name estimate" = $estimate
    }
}

Write-Host "`n=== Summary ===" -ForegroundColor Cyan
$summary | Format-Table -AutoSize
