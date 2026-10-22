<#
.SYNOPSIS
  Copies the chess-pack piece set and board (in .\themes\ next to this script) into ChessX.
  Then restart ChessX and choose "chess-pack" for pieces and board in Edit > Preferences > Board.
  (Or double-click install_chessx_theme.bat, which runs this.)
#>
$source = Join-Path $PSScriptRoot "themes"
$chessxThemes = Join-Path $env:LOCALAPPDATA "chessx\chessx\data\themes"

# ChessX looks in themes\outline\ or themes\shadow\ when that piece effect is chosen;
# the art already has outlines and shading, so the same sheet goes in all three
foreach ($effect in "", "outline", "shadow") {
    $dir = Join-Path $chessxThemes $effect
    New-Item -ItemType Directory -Force $dir | Out-Null
    Copy-Item (Join-Path $source "chess-pack.png") $dir -Force
}
New-Item -ItemType Directory -Force (Join-Path $chessxThemes "boards") | Out-Null
Copy-Item (Join-Path $source "boards\chess-pack.png") (Join-Path $chessxThemes "boards") -Force

Write-Host "Installed into $chessxThemes"
Write-Host "Restart ChessX, then Edit > Preferences > Board: choose chess-pack for pieces and board."
