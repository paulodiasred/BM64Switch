#!/usr/bin/env python3
"""Writes a starting point for translations/<language>-dialogue.txt from the dumped dialogue images.

Usage: dialogue_template.py <filedump dir> <out .txt> [file id ...]

For each dialogue file: one "line" entry per text line found (rows y0-y1, from the line's first pixel to the right
edge), with the columns of inline icons (pixels in other colors than the text) listed as {x0-x1} placeholders.
The text itself has to be transcribed and translated by hand.
"""
import os
import struct
import sys
from collections import Counter

sys.path.insert(0, os.path.dirname(__file__))
from dialogue_layout import DIALOGUE_FILES, find_lines, load_image  # noqa: E402


def icon_runs(w, rows, ink, y0, y1, x0):
    cols = [x for x in range(x0, w) if any(rows[y][x] not in (0, ink) for y in range(y0, y1 + 1))]
    runs = []
    for x in cols:
        if runs and x - runs[-1][1] <= 2:
            runs[-1][1] = x
        else:
            runs.append([x, x])
    return [r for r in runs if r[1] - r[0] >= 3]


def main():
    dump_dir, out_path = sys.argv[1], sys.argv[2]
    ids = [int(v) for v in sys.argv[3:]] or DIALOGUE_FILES
    out = open(out_path, "w", encoding="utf-8")
    for file_id in ids:
        w, h, rows, size = load_image(os.path.join(dump_dir, "%04d.bin" % file_id))
        ink = Counter(v for r in rows for v in r if v).most_common(1)[0][0]
        lines, art = find_lines(w, h, rows)
        out.write("[%d]\n" % file_id)
        for ln in lines:
            y0, y1 = ln["y0"], ln["y1"]
            # Bands taller than one line hold two lines (icons with labels join them): split them.
            slots = [(y0, min(y0 + 12, h - 1))] if y1 - y0 <= 15 else [(y, min(y + 12, y1)) for y in range(y1 - 12 - 16 * ((y1 - y0 - 12) // 16), y1, 16)]
            for s0, s1 in slots:
                icons = icon_runs(w, rows, ink, s0, s1, ln["x0"])
                hint = " ".join("{%d-%d}" % tuple(r) for r in icons)
                out.write("# en: \n")
                out.write("line %d %d %d 255: %s\n" % (s0, s1, ln["x0"], hint))
        out.write("\n")


if __name__ == "__main__":
    main()
