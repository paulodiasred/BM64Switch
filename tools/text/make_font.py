#!/usr/bin/env python3
"""Builds translations/dialogue_font.bin, the bitmap font the dialogue translation is drawn with.

Usage: make_font.py <out file>   (runs in the bm64switch-host image, which has Pillow and the Liberation fonts)

Format (all little-endian):
  "BMF1", u8 font count, then per font:
    u8 line height, u8 ascent, then 224 glyphs for code points 32-255 (Latin-1):
      u8 advance (1/4 pixels), u8 width, u8 height, s8 x offset, s8 y offset (from the top of the line),
      u32 bitmap offset
    u32 bitmap size, bitmap (1 byte per pixel, 0 or 1)
Font 0 is the dialogue text, font 1 the small labels in tutorial diagrams.

The Liberation fonts are licensed under the SIL Open Font License 1.1 (see translations/FONT-LICENSE.txt).
"""
import struct
import sys

from PIL import Image, ImageDraw, ImageFont

FONT_DIR = "/usr/share/fonts/truetype/liberation/"
FONTS = [
    (FONT_DIR + "LiberationSerif-Regular.ttf", 13, 16, True),
    (FONT_DIR + "LiberationMono-Regular.ttf", 9, 10, False),
]


def thin_stems(canvas, min_run=3):
    """The game's dialogue font has 1-pixel strokes, but hinting gives some letters 2-pixel vertical stems (they
    look bold). Narrows stems that are exactly 2 pixels wide for at least `min_run` rows."""
    w, h = canvas.size
    src = canvas.load()
    out = canvas.copy()
    dst = out.load()
    on = lambda x, y: 0 <= x < w and src[x, y] >= 128
    for x in range(w - 1):
        y = 0
        while y < h:
            stem = lambda yy: on(x, yy) and on(x + 1, yy) and not on(x - 1, yy) and not on(x + 2, yy)
            if not stem(y):
                y += 1
                continue
            start = y
            while y < h and stem(y):
                y += 1
            if y - start >= min_run:
                for yy in range(start, y):
                    dst[x + 1, yy] = 0
    return out


def build_font(path, size, line_height, thin):
    font = ImageFont.truetype(path, size)
    ascent, _ = font.getmetrics()
    glyphs = []
    bitmap = bytearray()
    for code in range(32, 256):
        ch = chr(code)
        advance = int(round(font.getlength(ch) * 4))  # in quarter pixels
        canvas = Image.new("L", (size * 2, line_height * 2), 0)
        draw = ImageDraw.Draw(canvas)
        draw.fontmode = "1"  # no anti-aliasing, like the game's pixel font
        draw.text((size // 2, line_height // 2), ch, fill=255, font=font)
        if thin:
            canvas = thin_stems(canvas)
        box = canvas.getbbox()
        if box is None or ch == " ":
            glyphs.append((advance, 0, 0, 0, 0, len(bitmap)))
            continue
        x0, y0, x1, y1 = box
        offset = len(bitmap)
        for y in range(y0, y1):
            for x in range(x0, x1):
                bitmap.append(1 if canvas.getpixel((x, y)) >= 128 else 0)
        # The game's font leaves about a pixel between letters; some Liberation letters (E, L, V...) leave more.
        ink_right = (x0 - size // 2) + (x1 - x0)
        advance = min(advance, int(round((ink_right + 1.25) * 4)))
        glyphs.append((advance, x1 - x0, y1 - y0, x0 - size // 2, y0 - line_height // 2, offset))
    out = struct.pack("<BB", line_height, ascent)
    for g in glyphs:
        out += struct.pack("<BBBbbI", *g)
    out += struct.pack("<I", len(bitmap)) + bytes(bitmap)
    return out


def main():
    data = b"BMF1" + struct.pack("<B", len(FONTS))
    for path, size, line_height, thin in FONTS:
        data += build_font(path, size, line_height, thin)
    open(sys.argv[1], "wb").write(data)


if __name__ == "__main__":
    main()
