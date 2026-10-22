<#
.SYNOPSIS
  Turns Joszs's "Chess Pieces and Board Pack" (CC BY 4.0, https://joszs.itch.io/chess-pack)
  into a ChessX piece set and board texture.

.EXAMPLE
  .\play\gui-theme\make_chessx_theme.ps1            # rebuild play\gui-theme\themes\ from .\chess-pack\
  .\play\gui-theme\make_chessx_theme.ps1 -Install   # ...and copy it into ChessX's data folder

  The pack is unzipped into .\chess-pack\ (not committed). ChessX wants:
  - pieces: one PNG in a 6 x 2 grid, white on top, black below, each row
    rook, knight, bishop, queen, king, pawn  ->  themes\chess-pack.png
  - board:  one light square above one dark square        ->  themes\boards\chess-pack.png
  Then pick "chess-pack" for pieces and board in ChessX: Edit > Preferences > Board.
#>
param(
    [string]$PackDir = "",
    [int]$TileSize = 256,
    [switch]$Install
)

Add-Type -AssemblyName System.Drawing
$root = (Resolve-Path "$PSScriptRoot\..\..").Path
if (-not $PackDir) { $PackDir = Join-Path $root "chess-pack" }
if (-not (Test-Path (Join-Path $PackDir "board.png"))) {
    Write-Error "Unzip chess-pack.zip into '$PackDir' first"; exit 1
}
$themes = Join-Path $PSScriptRoot "themes"
New-Item -ItemType Directory -Force (Join-Path $themes "boards") | Out-Null

function New-Canvas([int]$width, [int]$height) {
    $bitmap = New-Object System.Drawing.Bitmap $width, $height, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $graphics.Clear([System.Drawing.Color]::Transparent)
    return $bitmap, $graphics
}

# --- Pieces: 6 x 2 sprite sheet ---
$order = "rook", "knight", "bishop", "queen", "king", "pawn"
$sheet, $g = New-Canvas ($TileSize * 6) ($TileSize * 2)
for ($row = 0; $row -lt 2; $row++) {
    $colour = @("white", "black")[$row]
    for ($col = 0; $col -lt 6; $col++) {
        $image = [System.Drawing.Image]::FromFile((Join-Path $PackDir "chess-$($order[$col])-$colour.png"))
        $g.DrawImage($image, $col * $TileSize, $row * $TileSize, $TileSize, $TileSize)
        $image.Dispose()
    }
}
$g.Dispose()
$sheet.Save((Join-Path $themes "chess-pack.png"), [System.Drawing.Imaging.ImageFormat]::Png)
$sheet.Dispose()

# --- Board: the a8 (light) and b8 (dark) squares, each with its top and left outline,
# so squares placed side by side make single grid lines. Grid lines in board.png are at
# x = 18, 272, 523 and y = 25, 274.
$board = [System.Drawing.Image]::FromFile((Join-Path $PackDir "board.png"))
$texture, $g = New-Canvas $TileSize ($TileSize * 2)
$unit = [System.Drawing.GraphicsUnit]::Pixel
$g.DrawImage($board, (New-Object System.Drawing.Rectangle 0, 0, $TileSize, $TileSize),
    (New-Object System.Drawing.Rectangle 18, 25, 248, 248), $unit)
$g.DrawImage($board, (New-Object System.Drawing.Rectangle 0, $TileSize, $TileSize, $TileSize),
    (New-Object System.Drawing.Rectangle 272, 25, 246, 248), $unit)
$g.Dispose()
$board.Dispose()
$texture.Save((Join-Path $themes "boards\chess-pack.png"), [System.Drawing.Imaging.ImageFormat]::Png)
$texture.Dispose()

Write-Host "Wrote $themes\chess-pack.png and $themes\boards\chess-pack.png"

if ($Install) {
    $chessxThemes = Join-Path $env:LOCALAPPDATA "chessx\chessx\data\themes"
    New-Item -ItemType Directory -Force (Join-Path $chessxThemes "boards") | Out-Null
    Copy-Item (Join-Path $themes "chess-pack.png") $chessxThemes -Force
    Copy-Item (Join-Path $themes "boards\chess-pack.png") (Join-Path $chessxThemes "boards") -Force
    Write-Host "Installed into $chessxThemes (restart ChessX, then Edit > Preferences > Board)"
}
