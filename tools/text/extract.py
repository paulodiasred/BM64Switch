#!/usr/bin/env python3
"""Lists the game's text strings from the decompressed ROM, in the format of translations/*.txt.

Usage: extract.py bm64.decomp.us.z64 > strings.txt

Scans the areas of the main code data and of the menu overlays where the game's strings live and prints every
NUL-terminated run of text characters. The output is the template for a translation file; it's derived from the
ROM, so it must not be committed.
"""
import sys

# (ROM offset, length, description). Found by searching the decompressed ROM for encoded strings.
REGIONS = [
    (0xBA000, 0x4000, "main: stage names, results, pause, game over, saves, credits"),
    (0x824000, 0x2000, "ovl_2_subovl_6: Controller Pak manager"),
    (0x827000, 0x2800, "ovl_3_subovl_1: title menu"),
    (0x834000, 0x2800, "ovl_3_subovl_3: battle stages"),
    (0x83A000, 0x2800, "ovl_3_subovl_4: Custom parts"),
    (0x83F000, 0x2800, "ovl_3_subovl_5: Options"),
    (0x845000, 0x2800, "ovl_3_subovl_6: file select"),
    (0x84A000, 0x2800, "ovl_3_subovl_7: battle config"),
]

PUNCT = {0x0F: " ", 0xF6: "!", 0xF7: "?", 0xF8: ".", 0xF9: "-", 0xFB: "/", 0xFF: "\\n"}


def decode(b):
    if 0x1A <= b <= 0x33:
        return chr(ord("A") + b - 0x1A)
    if 0x3A <= b <= 0x53:
        return chr(ord("a") + b - 0x3A)
    if 0x10 <= b <= 0x19:
        return chr(ord("0") + b - 0x10)
    return PUNCT.get(b)


def strings_in(data, start, length):
    i = start
    end = min(start + length, len(data))
    while i < end:
        j = i
        text = ""
        while j < end and decode(data[j]) is not None:
            text += decode(data[j])
            j += 1
        letters = sum(c.isalpha() for c in text)
        # A string ends with NUL and starts after a NUL (or another string's end).
        if j < end and data[j] == 0 and letters >= 2 and (i == 0 or data[i - 1] == 0):
            yield i, text
        i = j + 1 if j > i else i + 1


def main():
    data = open(sys.argv[1], "rb").read()
    seen = set()
    for start, length, description in REGIONS:
        print(f"# ===== {description} =====\n")
        for offset, text in strings_in(data, start, length):
            if text in seen:
                continue
            seen.add(text)
            print(f"# ROM 0x{offset:06X}\nen: {text}\npt: \n")


if __name__ == "__main__":
    main()
