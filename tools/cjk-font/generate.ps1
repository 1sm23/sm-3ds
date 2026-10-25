$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$text = Get-Content (Join-Path $root 'romfs\lang\zh.txt') -Raw -Encoding UTF8
$codes = [System.Collections.Generic.HashSet[int]]::new()
foreach ($ch in $text.ToCharArray()) {
  $code = [int][char]$ch
  if ($code -ge 0x4e00 -and $code -le 0x9fff) { [void]$codes.Add($code) }
}

$fontCache = @{}
function Get-Rows([int]$code, [int]$size, [int]$width, [int]$height, [int]$x, [int]$y) {
  if (!$fontCache.ContainsKey($size)) {
    $fontCache[$size] = [System.Drawing.Font]::new('Microsoft YaHei', $size,
      [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
  }
  $bmp = [System.Drawing.Bitmap]::new($width, $height)
  $graphics = [System.Drawing.Graphics]::FromImage($bmp)
  $graphics.Clear([System.Drawing.Color]::Black)
  $graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::SingleBitPerPixelGridFit
  $graphics.DrawString([char]$code, $fontCache[$size], [System.Drawing.Brushes]::White, $x, $y)
  $rows = @()
  for ($row = 0; $row -lt $height; $row++) {
    $bits = 0
    for ($column = 0; $column -lt $width; $column++) {
      if ($bmp.GetPixel($column, $row).R -gt 128) { $bits = $bits -bor (1 -shl ($width - 1 - $column)) }
    }
    $rows += ('0x{0:X2}' -f $bits)
  }
  $graphics.Dispose()
  $bmp.Dispose()
  $rows -join ', '
}

$out = [System.Text.StringBuilder]::new()
[void]$out.AppendLine('#include "cjk_font.h"')
[void]$out.AppendLine('')
[void]$out.AppendLine('static const CjkGlyph kGlyphs[] = {')
foreach ($code in ($codes | Sort-Object)) {
  $ui = Get-Rows $code 7 7 7 0 -2
  $small = Get-Rows $code 8 8 8 -1 -2
  $raw = Get-Rows $code 16 16 16 -1 -3
  $large = ($raw -split ', ' | % {
    $value = [convert]::ToInt32($_, 16); $bits = 0
    for ($column = 0; $column -lt 8; $column++) {
      if (($value -band (1 -shl (15 - $column * 2))) -or ($value -band (1 -shl (14 - $column * 2)))) {
        $bits = $bits -bor (1 -shl (7 - $column))
      }
    }
    '0x{0:X2}' -f $bits
  }) -join ', '
  [void]$out.AppendLine(('  {{ 0x{0:X4}, {{ {1} }}, {{ {2} }}, {{ {3} }} }},' -f $code, $ui, $small, $large))
}
[void]$out.AppendLine('};')
[void]$out.AppendLine('')
[void]$out.AppendLine('const CjkGlyph *CjkFont_Find(unsigned code) {')
[void]$out.AppendLine('  for (unsigned i = 0; i < sizeof(kGlyphs) / sizeof(kGlyphs[0]); i++)')
[void]$out.AppendLine('    if (kGlyphs[i].code == code) return &kGlyphs[i];')
[void]$out.AppendLine('  return 0;')
[void]$out.AppendLine('}')
[System.IO.File]::WriteAllText((Join-Path $root 'source\cjk_font.c'), $out.ToString(), [Text.Encoding]::UTF8)
