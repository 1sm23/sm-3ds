#!/usr/bin/env python3
"""Turns an AI-extended 400x240 picture into the file the port reads (source/scene_art.h).

Usage: to_console.py PICTURE.png ORIGINAL_256x224.png PALETTE.bin BACKDROP.txt OUT.bin

  PALETTE.bin  the screen's CGRAM as dumped (256 BGR555 words): the art is written as indices into it, so it follows the
               game's palette fades and animation as the plane does.
  BACKDROP.txt the backdrop colour per screen row (224 lines, 5-bit G = B, R = 0): a pixel of exactly that colour is
               written transparent. The game draws that gradient by itself, per scanline, behind the plane.

Checks the AI's rules (the original rectangle unchanged, no colour the original does not use). Writes 400x240 bytes,
rows from the top: 0 = transparent, else a CGRAM index (the row's gradient taken off: the game's colour math adds it). Only the side margins (x < 72, x >= 328) are written: inside
the plane's own 256 columns the game's picture, or nothing, is what it shows. No pixel of the game's art ships.
Neither the palette nor the backdrop table belong in the repository (they are the game's data).
"""
import struct, sys
from PIL import Image

X, Y, W, H = 72, 8, 256, 224


def main():
    pic = Image.open(sys.argv[1]).convert("RGB")
    orig = Image.open(sys.argv[2]).convert("RGB")
    cg = open(sys.argv[3], "rb").read()
    backdrop = [int(l) for l in open(sys.argv[4]) if l.strip() and not l.startswith("#")]
    if pic.size != (400, 240) or orig.size != (W, H) or len(cg) != 512 or len(backdrop) != H:
        sys.exit("sizes: picture %s, original %s, palette %d bytes, backdrop %d rows" % (pic.size, orig.size, len(cg), len(backdrop)))
    changed = sum(1 for y in range(H) for x in range(W) if pic.getpixel((X + x, Y + y)) != orig.getpixel((x, y)))
    palette = set(orig.getdata())
    off = len({p for p in pic.getdata()} - palette)
    print("changed pixels in the original area: %d, colours not in the original: %d" % (changed, off))
    if changed or off:
        sys.exit("the picture breaks the rules; fix it (paste the original back, restrict the colours)")
    index = {}
    for i in range(255, 0, -1):
        w = struct.unpack_from("<H", cg, i * 2)[0]
        index[(w & 31, (w >> 5) & 31, (w >> 10) & 31)] = i
    out = bytearray(400 * 240)
    clear = nearest = 0
    for y in range(240):
        for x in range(400):
            if X <= x < X + W:
                continue
            r, g, b = (v >> 3 for v in pic.getpixel((x, y)))
            v = backdrop[min(max(y - Y, 0), H - 1)]
            if (r, g, b) == (0, v, v):
                clear += 1
                continue
            # The game adds the row's backdrop colour to everything (colour math): take it off, the port adds it again.
            b0 = (r, max(g - v, 0), max(b - v, 0))
            i = index.get(b0)
            if i is None:
                nearest += 1
                i = min(index.items(), key=lambda kv: sum((a - c) ** 2 for a, c in zip(kv[0], b0)))[1]
            out[y * 400 + x] = i
    open(sys.argv[5], "wb").write(out)
    print("wrote %s: %d backdrop pixels transparent, %d colours not in the palette (nearest used)" % (sys.argv[5], clear, nearest))


main()
