#!/usr/bin/env python3
"""Finds the text lines in the game's dialogue images, for the dialogue translation.

Usage: dialogue_layout.py <filedump dir> <out json> [<debug image dir>]

The dialogue images (files whose first section is a 256-pixel-wide 4bpp image, see DIALOGUE_FILES) are white text
with gray anti-aliasing on black, plus portraits, icons and small diagrams. Glyphs are small connected components;
anything bigger (portraits, icons, drawings) is left alone. Glyphs are grouped into lines by their vertical
position, and lines into paragraphs by spacing. The output only has coordinates (no image data), so it can be
committed: translation.cpp uses it to know where to erase and redraw text.
"""
import json
import os
import struct
import sys

DIALOGUE_FILES = [205, 206, 629, 630, 631, 632, 654, 655, 656, 657, 816, 817, 818, 819, 820, 821, 822, 823, 824,
                  843, 844, 856, 857, 858, 859, 860, 861, 863, 864, 865, 866, 867, 868]

INK = 1            # pixel value (0-15) that counts as part of a glyph (anything but the background)
MAX_GLYPH_H = 13


def load_image(path):
    d = open(path, "rb").read()
    w, h = struct.unpack(">HH", d[0x10:0x14])
    off = struct.unpack(">I", d[0x14:0x18])[0]
    px = []
    for b in d[off:off + w * h // 2]:
        px.append(b >> 4)
        px.append(b & 15)
    rows = [px[y * w:(y + 1) * w] for y in range(h)]
    return w, h, rows, len(d)


def components(w, h, rows):
    seen = [[False] * w for _ in range(h)]
    comps = []
    for y in range(h):
        for x in range(w):
            if seen[y][x] or rows[y][x] < INK:
                continue
            stack = [(x, y)]
            seen[y][x] = True
            x0 = x1 = x
            y0 = y1 = y
            count = 0
            while stack:
                cx, cy = stack.pop()
                count += 1
                x0, x1, y0, y1 = min(x0, cx), max(x1, cx), min(y0, cy), max(y1, cy)
                for dx in (-1, 0, 1):
                    for dy in (-1, 0, 1):
                        nx, ny = cx + dx, cy + dy
                        if 0 <= nx < w and 0 <= ny < h and not seen[ny][nx] and rows[ny][nx] >= INK:
                            seen[ny][nx] = True
                            stack.append((nx, ny))
            comps.append((x0, y0, x1, y1, count))
    return comps


def find_lines(w, h, rows):
    comps = components(w, h, rows)
    glyphs = [c for c in comps if c[3] - c[1] + 1 <= MAX_GLYPH_H]
    art = [c for c in comps if c not in glyphs]
    # Row coverage of glyph pixels; text lines are bands of rows with glyphs, separated by empty rows.
    coverage = [0] * h
    for x0, y0, x1, y1, _ in glyphs:
        for y in range(y0, y1 + 1):
            coverage[y] += 1
    bands = []
    y = 0
    while y < h:
        if coverage[y] == 0:
            y += 1
            continue
        start = y
        while y < h and coverage[y] > 0:
            y += 1
        bands.append([start, y - 1])
    # Merge bands closer than 2 rows (accents, descenders separated by a gap row).
    merged = []
    for b in bands:
        if merged and b[0] - merged[-1][1] <= 2 and b[1] - merged[-1][0] < MAX_GLYPH_H + 2:
            merged[-1][1] = b[1]
        else:
            merged.append(b)
    lines = []
    for y0, y1 in merged:
        members = [g for g in glyphs if g[1] >= y0 and g[3] <= y1]
        if sum(g[4] for g in members) < 12:
            continue  # stray dots
        lines.append({"y0": y0, "y1": y1, "x0": min(g[0] for g in members), "x1": max(g[2] for g in members)})
    return lines, art


def main():
    dump_dir, out_json = sys.argv[1], sys.argv[2]
    debug_dir = sys.argv[3] if len(sys.argv) > 3 else None
    result = {}
    for file_id in DIALOGUE_FILES:
        w, h, rows, size = load_image(os.path.join(dump_dir, "%04d.bin" % file_id))
        lines, art = find_lines(w, h, rows)
        result[str(file_id)] = {"width": w, "height": h, "size": size, "lines": lines,
                                "art": [list(a[:4]) for a in art if (a[2] - a[0]) * (a[3] - a[1]) >= 16]}
        if debug_dir:
            os.makedirs(debug_dir, exist_ok=True)
            img = [[v * 17 for v in r] for r in rows]
            rgb = bytearray()
            for y in range(h):
                for x in range(w):
                    v = img[y][x]
                    color = (v, v, v)
                    for ln in lines:
                        if ln["y0"] <= y <= ln["y1"] and ln["x0"] <= x <= ln["x1"] and v < 40:
                            color = (0, 60, 140)
                    rgb += bytes(color)
            open(os.path.join(debug_dir, "%04d.ppm" % file_id), "wb").write(b"P6\n%d %d\n255\n" % (w, h) + rgb)
    json.dump(result, open(out_json, "w"), indent=1)


if __name__ == "__main__":
    main()
