#!/usr/bin/env python3
"""Previews the dialogue translation without running the game.

Usage: preview_dialogue.py <filedump dir> <spec .txt> <font .bin> <out dir> [file id ...]

Applies translations/<language>-dialogue.txt to the dialogue images dumped with BM64_FILE_DUMP=1 the same way
src/game/dialogue_translation.cpp does, and writes <id>.png (original | translated, 2x) for each file, reporting
lines that don't fit. The images come from the ROM: keep them out of the repository.
"""
import os
import struct
import sys

from PIL import Image


def load_font(path):
    d = open(path, "rb").read()
    assert d[:4] == b"BMF1"
    pos = 5
    fonts = []
    for _ in range(d[4]):
        line_height, ascent = d[pos], d[pos + 1]
        pos += 2
        glyphs = []
        for _ in range(224):
            glyphs.append(struct.unpack_from("<BBBbbI", d, pos))
            pos += 9
        size = struct.unpack_from("<I", d, pos)[0]
        pos += 4
        fonts.append((glyphs, d[pos:pos + size]))
        pos += size
    return fonts


def load_spec(path):
    files = {}
    current = None
    for raw in open(path, encoding="utf-8"):
        line = raw.rstrip("\n").rstrip("\r")
        if not line or line.startswith("#"):
            continue
        if line.startswith("["):
            current = files.setdefault(int(line[1:line.index("]")]), [])
            continue
        head, _, text = line.partition(":")
        if text.startswith(" "):
            text = text[1:]
        kind, y0, y1, x0, x1 = head.split()
        current.append((kind, int(y0), int(y1), int(x0), int(x1), text))
    return files


def load_image(path):
    d = open(path, "rb").read()
    w, h = struct.unpack(">HH", d[0x10:0x14])
    off = struct.unpack(">I", d[0x14:0x18])[0]
    px = []
    for b in d[off:off + w * h // 2]:
        px += [b >> 4, b & 15]
    return w, h, px


def apply(w, h, px, ops, fonts, file_id):
    orig = list(px)
    get = lambda x, y: orig[y * w + x] if 0 <= x < w and 0 <= y < h else 0

    def put(x, y, v):
        if 0 <= x < w and 0 <= y < h:
            px[y * w + x] = v

    file_counts = [0] * 16
    for kind, y0, y1, x0, x1, _ in ops:
        if kind == "line":
            for y in range(max(y0, 0), min(y1, h - 1) + 1):
                for x in range(max(x0, 0), min(x1, w - 1) + 1):
                    file_counts[orig[y * w + x]] += 1
    text_color = max(range(1, 16), key=lambda c: (file_counts[c], -c))
    for kind, y0, y1, x0, x1, text in ops:
        counts = [0] * 16
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                counts[get(x, y)] += 1
        ink = max(range(1, 16), key=lambda c: (counts[c], -c))
        if counts[text_color] > 0:
            ink = text_color
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                if kind != "cleartext" or (0 <= x < w and 0 <= y < h and px[y * w + x] == text_color):
                    put(x, y, 0)
        if kind == "cleartext":
            for y in range(y0, y1 + 1):
                for x in range(x0, x1 + 1):
                    if get(x, y) != text_color:
                        continue
                    around = [0] * 16
                    for dy in (-1, 0, 1):
                        for dx in (-1, 0, 1):
                            nx, ny = x + dx, y + dy
                            v = px[ny * w + nx] if 0 <= nx < w and 0 <= ny < h else 0
                            if v not in (0, text_color):
                                around[v] += 1
                    if sum(around) >= 3:
                        put(x, y, max(range(16), key=lambda c: around[c]))
        if kind in ("clear", "cleartext"):
            continue
        glyphs, bitmap = fonts[1 if kind == "small" else 0]
        scale = 2 if kind == "big" else 1
        glyph = lambda c: glyphs[(c if 32 <= c <= 255 else ord("?")) - 32]
        origin_y = y0 - glyph(ord("H"))[4] * scale
        align = ""
        if text[:1] in (">", "^"):
            align, text = text[0], text[1:]

        def run(start, draw):
            pen = start * 4  # quarter pixels
            i = 0
            while i < len(text):
                if text[i] == "{":
                    end = text.index("}", i)
                    parts = text[i + 1:end].split()
                    a, b = (int(v) for v in parts[0].split("-"))
                    rows = (y0 - 5, y1 + 3)
                    if len(parts) > 1:
                        rows = tuple(int(v) for v in parts[1].split("-"))
                    x_out = (pen + 3) // 4
                    for x in range(a, b + 1):
                        if draw:
                            for y in range(rows[0], rows[1] + 1):
                                v = get(x, min(max(y, 0), h - 1))
                                if y0 <= y <= y1 or (v != 0 and v != ink):
                                    put(x_out, y, v)
                        x_out += 1
                    pen = (x_out + 1) * 4
                    i = end + 1
                    continue
                adv, gw, gh, xo, yo, off = glyph(ord(text[i]))
                if draw:
                    x = pen // 4
                    for gy in range(gh):
                        for gx in range(gw):
                            if bitmap[off + gy * gw + gx]:
                                for sy in range(scale):
                                    for sx in range(scale + (1 if scale > 1 else 0)):
                                        put(x + (xo + gx) * scale + sx, origin_y + (yo + gy) * scale + sy, ink)
                pen += adv * scale
                i += 1
            return (pen + 3) // 4

        width = run(0, False)
        start = x0
        if align == ">":
            start = x1 + 1 - width
        elif align == "^":
            start = x0 + (x1 - x0 + 1 - width) // 2
        pen = run(start, True)
        if pen - 1 > x1:
            print("file %d, line at y %d: %d px too wide: %s" % (file_id, y0, pen - 1 - x1, text))


def to_image(w, h, px):
    # Text color (1) white, other colors as grays.
    lut = [0] + [255] + [min(255, 60 + v * 12) for v in range(2, 16)]
    im = Image.new("L", (w, h))
    im.putdata([lut[v] for v in px])
    return im


def main():
    dump_dir, spec_path, font_path, out_dir = sys.argv[1:5]
    only = {int(v) for v in sys.argv[5:]}
    fonts = load_font(font_path)
    os.makedirs(out_dir, exist_ok=True)
    for file_id, ops in sorted(load_spec(spec_path).items()):
        if only and file_id not in only:
            continue
        w, h, px = load_image(os.path.join(dump_dir, "%04d.bin" % file_id))
        before = to_image(w, h, px)
        apply(w, h, px, ops, fonts, file_id)
        after = to_image(w, h, px)
        out = Image.new("L", (w * 2 + 8, h), 40)
        out.paste(before, (0, 0))
        out.paste(after, (w + 8, 0))
        out.resize((out.width * 2, out.height * 2), Image.NEAREST).save(os.path.join(out_dir, "%04d.png" % file_id))


if __name__ == "__main__":
    main()
