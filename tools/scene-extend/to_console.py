#!/usr/bin/env python3
"""Turns an AI-extended 400x240 picture into the file the port reads (source/scene_art.h).

Usage: to_console.py PICTURE.png ORIGINAL_256x224.png PALETTE.bin MATH.txt OUT.bin

  PALETTE.bin  the screen's CGRAM as dumped (256 BGR555 words): the art is written as indices into it, so it follows the
               game's palette fades and animation as the plane does.
  MATH.txt     the colour math per screen row, 224 lines "SUB R G B": SUB 1 = subtracted, 0 = added, the fixed colour
               in 5 bits. The game's math is on the plane and the backdrop; the port applies it to the art again, so
               the art is written without it. A pixel that is the row's backdrop (black where subtracted, the fixed
               colour where added) is written transparent: the game draws the backdrop by itself.

Checks the AI's rules (the original rectangle unchanged, no colour the original does not use). Writes 400x240 16-bit
words (little endian), rows from the top: 0 = transparent; 1..255 a CGRAM index (it follows the palette); 0x8000 | BGR555
a fixed colour, for the colours the palette does not hold (the game changes it down the screen). The row's gradient is
taken off in both. Only the side margins (x < 72, x >= 328) are written: inside
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
    math = [tuple(int(v) for v in l.split()) for l in open(sys.argv[4]) if l.strip() and not l.startswith("#")]
    if pic.size != (400, 240) or orig.size != (W, H) or len(cg) != 512 or len(math) != H:
        sys.exit("sizes: picture %s, original %s, palette %d bytes, math %d rows" % (pic.size, orig.size, len(cg), len(math)))
    changed = sum(1 for y in range(H) for x in range(W) if pic.getpixel((X + x, Y + y)) != orig.getpixel((x, y)))
    palette = set(orig.getdata())
    off = len({p for p in pic.getdata()} - palette)
    print("changed pixels in the original area: %d, colours not in the original: %d" % (changed, off))
    if changed or off:
        sys.exit("the picture breaks the rules; fix it (paste the original back, restrict the colours)")
    # A row the original dithers at its edge (a pixel of one colour, one of another, ...: the floor's edge) the AI
    # continues as one colour: take the pattern on into the margin, in the original's phase (8 pixels must follow it).
    dithered = 0
    for y in range(240):
        r = min(max(y - Y, 0), H - 1)
        for xs, margin in ((range(0, 8), range(0, X)), (range(W - 8, W), range(X + W, 400))):
            a, b = orig.getpixel((xs[0], r)), orig.getpixel((xs[1], r))
            if a == b or any(orig.getpixel((x, r)) != (a if (x - xs[0]) % 2 == 0 else b) for x in xs):
                continue
            dithered += 1
            for x in margin:
                if pic.getpixel((x, y)) in (a, b):
                    pic.putpixel((x, y), a if (x - X - xs[0]) % 2 == 0 else b)
    print("zipper edges continued into the margins: %d" % dithered)
    index = {}
    for i in range(255, 0, -1):
        w = struct.unpack_from("<H", cg, i * 2)[0]
        index[(w & 31, (w >> 5) & 31, (w >> 10) & 31)] = i
    out = bytearray(400 * 240 * 2)
    clear = nearest = 0
    for y in range(240):
        for x in range(400):
            if X <= x < X + W:
                continue
            r, g, b = (v >> 3 for v in pic.getpixel((x, y)))
            sub, fr, fg, fb = math[min(max(y - Y, 0), H - 1)]
            c = (r, g, b)
            if sub:
                if c == (0, 0, 0):
                    clear += 1
                    continue
                b0 = (min(r + fr, 31), min(g + fg, 31), min(b + fb, 31))
            else:
                if c == (fr, fg, fb):
                    clear += 1
                    continue
                b0 = (max(r - fr, 0), max(g - fg, 0), max(b - fb, 0))
            i = index.get(b0)
            if i is None:
                nearest += 1
                i = 0x8000 | b0[2] << 10 | b0[1] << 5 | b0[0]
            struct.pack_into("<H", out, (y * 400 + x) * 2, i)
    open(sys.argv[5], "wb").write(out)
    print("wrote %s: %d backdrop pixels transparent, %d colours not in the palette (fixed colours)" % (sys.argv[5], clear, nearest))


main()
