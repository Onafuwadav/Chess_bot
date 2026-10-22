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

foreach ($level in $Elo) {
    Write-Host "`n=== $Name vs Stockfish (UCI_Elo $level): $Games games, $TimeControl ===" -ForegroundColor Cyan
    $pgn = Join-Path $results "vs_stockfish_$level.pgn"
    $scoreLine = ""; $eloLine = ""

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
            Write-Host $line
            if ($line -match "^Score of") { $scoreLine = $line }
            if ($line -match "^Elo difference") { $eloLine = $line }
        }

    # "Elo difference: -95.4 +/- 120.3, LOS: ..." -> estimated rating = level + difference
    $estimate = "?"
    if ($eloLine -match "Elo difference:\s*(-?[\d.]+|-?inf)\s*\+/-\s*([\d.]+|nan)") {
        if ($Matches[1] -notmatch "inf") { $estimate = "{0:N0} +/- {1}" -f ($level + [double]$Matches[1]), $Matches[2] }
        else { $estimate = "out of range (won or lost every game)" }
    }
    $summary += [pscustomobject]@{ "Stockfish Elo" = $level; "Result (W-L-D)" = ($scoreLine -replace "^Score of .*?: ", ""); "$Name estimate" = $estimate }
}

Write-Host "`n=== Summary ===" -ForegroundColor Cyan
$summary | Format-Table -AutoSize
