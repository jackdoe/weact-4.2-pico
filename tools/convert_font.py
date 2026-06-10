#!/usr/bin/env python3
"""Convert a TTF/OTF monospace font to a packed 1-bit bitmap C source.

    python3 tools/convert_font.py [--bold] <font.otf> <size_px> <symbol_name> > src/<file>.c

Emits a `const gfx_font_t <symbol_name>` plus its glyph bitmap. Renders the
printable ASCII range 0x20..0x7E with anti-aliasing thresholded to mono
(>=128 = ink), then crops the cell to the tight common ink bounds across all
glyphs so no flash is spent on empty rows. With --bold, each glyph is
horizontally dilated by 1 px so thin strokes read clearly on e-paper.
"""

import sys
from PIL import Image, ImageDraw, ImageFont


def convert(font_path: str, size: int, symbol: str, first: int = 0x20, last: int = 0x7E, bold: bool = False):
    font = ImageFont.truetype(font_path, size)
    ascent, descent = font.getmetrics()
    cell_h = ascent + descent
    cell_w = int(round(font.getlength("M")))
    if bold:
        cell_w += 1
    stride = (cell_w + 7) // 8

    glyphs = []
    for c in range(first, last + 1):
        img = Image.new("L", (cell_w, cell_h), 0)
        draw = ImageDraw.Draw(img)
        draw.text((0, 0), chr(c), font=font, fill=255)

        rows = []
        for y in range(cell_h):
            row = [img.getpixel((x, y)) >= 128 for x in range(cell_w)]
            if bold:
                for x in range(cell_w - 1, 0, -1):
                    if row[x - 1]:
                        row[x] = True
            rows.append(row)
        glyphs.append(rows)

    top = cell_h
    bottom = -1
    for rows in glyphs:
        for y, row in enumerate(rows):
            if any(row):
                top = min(top, y)
                bottom = max(bottom, y)
    if bottom < top:
        top, bottom = 0, cell_h - 1
    cell_h = bottom - top + 1

    bitmap = bytearray()
    for rows in glyphs:
        for row in rows[top : bottom + 1]:
            packed = bytearray(stride)
            for x, on in enumerate(row):
                if on:
                    packed[x >> 3] |= 0x80 >> (x & 7)
            bitmap.extend(packed)

    out = []
    out.append("#include <stdint.h>")
    out.append('#include "gfx.h"')
    out.append("")
    out.append(f"static const uint8_t {symbol}_data[{len(bitmap)}] = {{")
    for i in range(0, len(bitmap), 12):
        chunk = bitmap[i : i + 12]
        out.append("    " + ", ".join(f"0x{b:02X}" for b in chunk) + ",")
    out.append("};")
    out.append("")
    out.append(f"const gfx_font_t {symbol} = {{")
    out.append(f"    .w     = {cell_w},")
    out.append(f"    .h     = {cell_h},")
    out.append(f"    .first = 0x{first:02X},")
    out.append(f"    .last  = 0x{last:02X},")
    out.append(f"    .bitmap = {symbol}_data,")
    out.append("};")
    return "\n".join(out)


if __name__ == "__main__":
    args = sys.argv[1:]
    bold = False
    if args and args[0] == "--bold":
        bold = True
        args = args[1:]
    if len(args) != 3:
        sys.exit(f"usage: {sys.argv[0]} [--bold] <font> <size_px> <symbol>")
    print(convert(args[0], int(args[1]), args[2], bold=bold))
