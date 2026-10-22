<#
.SYNOPSIS
  Makes dist\Chess_bot-windows.zip: the engine, the terminal game and the ChessX theme,
  ready to play without building anything. GitHub Actions runs this for each release.

.EXAMPLE
  cmake --build build
  .\scripts\package.ps1 -Version v1.0
#>
param(
    [string]$Version = "dev",
    [string]$BuildDir = ""
)

$ErrorActionPreference = "Stop"
$root = (Resolve-Path "$PSScriptRoot\..").Path
if (-not $BuildDir) { $BuildDir = Join-Path $root "build" }

$dist = Join-Path $root "dist"
$stage = Join-Path $dist "Chess_bot"
if (Test-Path $dist) { Remove-Item -Recurse -Force $dist }
New-Item -ItemType Directory -Force (Join-Path $stage "chessx-theme") | Out-Null

foreach ($exe in "chess_uci.exe", "chess_cli.exe") {
    $path = Join-Path $BuildDir $exe
    if (-not (Test-Path $path)) { throw "Missing $path - build first (cmake --build build)" }
    Copy-Item $path $stage
}

$theme = Join-Path $root "play\gui-theme"
Copy-Item -Recurse (Join-Path $theme "themes") (Join-Path $stage "chessx-theme")
foreach ($file in "install_chessx_theme.ps1", "install_chessx_theme.bat", "CREDITS.md") {
    Copy-Item (Join-Path $theme $file) (Join-Path $stage "chessx-theme")
}

@"
Chess_bot $Version
https://github.com/Onafuwadav/Chess_bot

PLAY IN CHESSX (recommended)
1. Install ChessX: https://chessx.sourceforge.io/
2. In ChessX: Edit > Preferences > Engines > Add, protocol UCI,
   command: the full path to chess_uci.exe in this folder.
3. Optional hand-drawn pieces and board: double-click chessx-theme\install_chessx_theme.bat,
   restart ChessX, then Edit > Preferences > Board and choose "chess-pack" for pieces and board.
4. Start a game against the engine (Play menu, or File > New game then choose the engine).

PLAY IN OTHER GUIS
chess_uci.exe works in any UCI chess GUI, e.g. Cute Chess, Arena or En Croissant.

PLAY IN THE TERMINAL
Double-click chess_cli.exe. Moves are four digits: from x, from y, to x, to y, using the
numbers printed around the board (e.g. 4244). Add q/r/b/n to choose a promotion piece.
Type h for help.

Art in chessx-theme: "Chess Pieces and Board Pack" by Joszs, CC BY 4.0 (see CREDITS.md).
"@ | Set-Content -Encoding UTF8 (Join-Path $stage "HOW_TO_PLAY.txt")

$zip = Join-Path $dist "Chess_bot-windows.zip"
Compress-Archive -Path $stage -DestinationPath $zip
Write-Host "Wrote $zip"
