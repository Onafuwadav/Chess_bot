<#
.SYNOPSIS
  Summarises every game in testing\match\results\*.pgn: for each of your engines (any player
  whose name doesn't start with "Stockfish"), its wins, losses and draws against each opponent,
  split by time control, with a rating estimate.

.EXAMPLE
  .\testing\match\summary.ps1                        # everything
  .\testing\match\summary.ps1 -TimeControl "120+1"   # only games at 120+1
  .\testing\match\summary.ps1 -Engine Chess_bot_v2   # only one engine

  Unfinished games (result "*") are skipped. Name each engine version differently
  (match.ps1 -Name ...) to compare versions.
#>
param(
    [string]$ResultsDir = "",
    [string]$TimeControl = "",
    [string]$Engine = ""
)

if (-not $ResultsDir) { $ResultsDir = Join-Path $PSScriptRoot "results" }
$files = Get-ChildItem $ResultsDir -Filter *.pgn -ErrorAction SilentlyContinue
if (-not $files) { Write-Error "No .pgn files in $ResultsDir"; exit 1 }

# --- Read the White / Black / Result / TimeControl tags of every game ---
$games = New-Object System.Collections.Generic.List[object]
foreach ($file in $files) {
    $tags = @{}
    # Open with sharing allowed, so files a running match is still writing can be read
    $stream = New-Object System.IO.FileStream($file.FullName, [System.IO.FileMode]::Open,
        [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
    $reader = New-Object System.IO.StreamReader($stream)
    $lines = $reader.ReadToEnd() -split "`r?`n"
    $reader.Close()
    foreach ($line in $lines) {
        if ($line -match '^\[(\w+)\s+"(.*)"\]') {
            if ($Matches[1] -eq "Event") { $tags = @{} } # each game starts with [Event ...]
            $tags[$Matches[1]] = $Matches[2]
            if ($Matches[1] -eq "TimeControl" -and $tags.ContainsKey("Result")) {
                $games.Add([pscustomobject]@{ White = $tags.White; Black = $tags.Black; Result = $tags.Result; TimeControl = $tags.TimeControl })
            }
        }
    }
}

# --- Tally from each of our engines' point of view ---
$tally = @{}   # "engine|timecontrol|opponent" -> W, L, D
foreach ($g in $games) {
    if ($g.Result -notin "1-0", "0-1", "1/2-1/2") { continue }
    if ($TimeControl -and $g.TimeControl -ne $TimeControl) { continue }
    foreach ($side in "White", "Black") {
        $me = $g.$side
        $opponent = if ($side -eq "White") { $g.Black } else { $g.White }
        if ($me -like "Stockfish*") { continue }
        if ($Engine -and $me -ne $Engine) { continue }
        $key = "$me|$($g.TimeControl)|$opponent"
        if (-not $tally.ContainsKey($key)) { $tally[$key] = @{ W = 0; L = 0; D = 0; White = 0; WhitePts = 0.0; Black = 0; BlackPts = 0.0 } }
        $won = ($side -eq "White" -and $g.Result -eq "1-0") -or ($side -eq "Black" -and $g.Result -eq "0-1")
        if ($g.Result -eq "1/2-1/2") { $tally[$key].D++; $points = 0.5 } elseif ($won) { $tally[$key].W++; $points = 1.0 } else { $tally[$key].L++; $points = 0.0 }
        # Games and points per colour, for the "As White" / "As Black" scores
        $tally[$key].$side++
        $tally[$key]["${side}Pts"] += $points
    }
}
if ($tally.Count -eq 0) { Write-Host "No finished games match."; exit 0 }

# Elo difference for a score fraction: -400 * log10(1/score - 1)
function EloDiff([double]$score) { return -400 * [math]::Log10(1 / $score - 1) }

# Rating of the opponent from its name, e.g. Stockfish_1500 -> 1500 (else none)
function OpponentRating([string]$name) { if ($name -match '_(\d{3,4})$') { return [int]$Matches[1] } return $null }

# --- Print one table per engine and time control ---
$groups = $tally.Keys | ForEach-Object { $p = $_ -split '\|'; [pscustomobject]@{ Engine = $p[0]; TC = $p[1]; Opponent = $p[2]; Key = $_ } } |
    Group-Object Engine, TC | Sort-Object Name

foreach ($group in $groups) {
    $engineName = $group.Group[0].Engine
    $tc = $group.Group[0].TC
    Write-Host "=== $engineName at $tc ===" -ForegroundColor Cyan

    $rows = @()
    $ratedGames = @()   # (opponent rating, points) for the overall estimate
    foreach ($entry in ($group.Group | Sort-Object { OpponentRating $_.Opponent }, Opponent)) {
        $t = $tally[$entry.Key]
        $n = $t.W + $t.L + $t.D
        $score = ($t.W + 0.5 * $t.D) / $n
        $rating = OpponentRating $entry.Opponent

        # 95% range for the Elo difference, from the spread of the game results
        $variance = ($t.W * [math]::Pow(1 - $score, 2) + $t.L * [math]::Pow($score, 2) + $t.D * [math]::Pow(0.5 - $score, 2)) / $n
        $margin = 1.96 * [math]::Sqrt($variance / $n)
        $diffText = "n/a (won or lost all)"
        $estimateText = ""
        if ($score -gt 0 -and $score -lt 1) {
            $diff = EloDiff $score
            $low = EloDiff ([math]::Max($score - $margin, 0.001))
            $high = EloDiff ([math]::Min($score + $margin, 0.999))
            $diffText = "{0:+0;-0} (+/- {1:0})" -f $diff, (($high - $low) / 2)
            if ($null -ne $rating) { $estimateText = "{0:0}" -f ($rating + $diff) }
        }
        $asWhite = if ($t.White -gt 0) { "{0:P0}" -f ($t.WhitePts / $t.White) } else { "-" }
        $asBlack = if ($t.Black -gt 0) { "{0:P0}" -f ($t.BlackPts / $t.Black) } else { "-" }
        $rows += [pscustomobject]@{
            Opponent = $entry.Opponent; Games = $n; W = $t.W; L = $t.L; D = $t.D
            Score = "{0:P0}" -f $score; "As White" = $asWhite; "As Black" = $asBlack
            "Elo diff" = $diffText; Estimate = $estimateText
        }
        if ($null -ne $rating) {
            for ($i = 0; $i -lt $t.W; $i++) { $ratedGames += , @($rating, 1.0) }
            for ($i = 0; $i -lt $t.L; $i++) { $ratedGames += , @($rating, 0.0) }
            for ($i = 0; $i -lt $t.D; $i++) { $ratedGames += , @($rating, 0.5) }
        }
    }
    # Format-Table pads with blank lines; trim them so the overall line sits under its table
    Write-Host ($rows | Format-Table -AutoSize | Out-String -Width 200).Trim()
    Write-Host ""

    # Overall estimate: the rating R at which the expected score against all the opponents
    # faced, 1 / (1 + 10^((opp - R) / 400)) per game, adds up to the points actually scored
    if ($ratedGames.Count -gt 0) {
        $points = ($ratedGames | ForEach-Object { $_[1] } | Measure-Object -Sum).Sum
        if ($points -le 0 -or $points -ge $ratedGames.Count) {
            Write-Host "Overall: can't estimate (won or lost every game)"
        } else {
            $lo = 0.0; $hi = 4000.0
            for ($i = 0; $i -lt 60; $i++) {
                $mid = ($lo + $hi) / 2
                $expected = 0.0
                foreach ($rg in $ratedGames) { $expected += 1 / (1 + [math]::Pow(10, ($rg[0] - $mid) / 400)) }
                if ($expected -lt $points) { $lo = $mid } else { $hi = $mid }
            }
            Write-Host ("Overall estimate from {0} games: about {1:0} (on Stockfish's UCI_Elo scale)" -f $ratedGames.Count, $lo) -ForegroundColor Green
        }
    }
    Write-Host "`n"   # two blank lines before the next engine / time control
}
