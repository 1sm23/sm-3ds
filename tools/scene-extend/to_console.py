#!/usr/bin/env python3
"""Turns an AI-extended 400x240 picture into the file the port reads (source/scene_art.h).

Usage: to_console.py PICTURE.png ORIGINAL_256x224.png OUT.bin [BACKDROP.txt]

BACKDROP.txt: the backdrop colour per screen row (title-backdrop.txt): a pixel of exactly that colour is written
transparent. The game draws that gradient by itself, per scanline, behind the plane: baked into the art it would not
move with the plane while it zooms, and would show twice where the art meets the plane.

Checks the AI's rules (the original rectangle unchanged, no colour the original does not use), then writes
400x240 RGBA5551, little endian, rows from the top, with the rectangle of the game's own 256x224 at (72, 8)
left empty: no pixel of the game's art ships, the port draws only the margins and the extra rows.
"""
import struct, sys
from PIL import Image

X, Y, W, H = 72, 8, 256, 224


def main():
    pic = Image.open(sys.argv[1]).convert("RGB")
    orig = Image.open(sys.argv[2]).convert("RGB")
    if pic.size != (400, 240) or orig.size != (W, H):
        sys.exit("sizes: picture %s (want 400x240), original %s (want 256x224)" % (pic.size, orig.size))
    changed = sum(1 for y in range(H) for x in range(W) if pic.getpixel((X + x, Y + y)) != orig.getpixel((x, y)))
    palette = set(orig.getdata())
    off = len({p for p in pic.getdata()} - palette)
    print("changed pixels in the original area: %d, colours not in the original: %d" % (changed, off))
    if changed or off:
        sys.exit("the picture breaks the rules; fix it (paste the original back, restrict the colours)")
    backdrop = None
    if len(sys.argv) > 4:
        backdrop = [int(l) for l in open(sys.argv[4]) if l.strip() and not l.startswith("#")]
        if len(backdrop) != H:
            sys.exit("%s: %d rows, want %d" % (sys.argv[4], len(backdrop), H))
    out = bytearray()
    clear = 0
    for y in range(240):
        for x in range(400):
            if X <= x < X + W and Y <= y < Y + H:
                out += struct.pack("<H", 0)
                continue
            r, g, b = pic.getpixel((x, y))
            if backdrop:
                v = backdrop[min(max(y - Y, 0), H - 1)]
                if (r >> 3, g >> 3, b >> 3) == (0, v, v):
                    out += struct.pack("<H", 0)
                    clear += 1
                    continue
            out += struct.pack("<H", (r >> 3) << 11 | (g >> 3) << 6 | (b >> 3) << 1 | 1)
    open(sys.argv[3], "wb").write(out)
    print("wrote %s (%d bytes, %d backdrop pixels left transparent)" % (sys.argv[3], len(out), clear))


main()
