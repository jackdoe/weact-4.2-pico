#!/usr/bin/env python3
"""Convert a TTF/OTF monospace font to a packed 1-bit bitmap C source.

    python3 tools/convert_font.py <font.otf> <size_px> <symbol_name> > src/<file>.c

Emits a `const gfx_font_t <symbol_name>` plus its glyph bitmap.
Renders the printable ASCII range 0x20..0x7E with anti-aliasing thresholded to
mono (>=128 = ink).
"""

import sys
from PIL import Image, ImageDraw, ImageFont


def convert(font_path: str, size: int, symbol: str, first: int = 0x20, last: int = 0x7E):
    font = ImageFont.truetype(font_path, size)
    ascent, descent = font.getmetrics()
    cell_h = ascent + descent
    cell_w = int(round(font.getlength("M")))
    stride = (cell_w + 7) // 8

    bitmap = bytearray()
    for c in range(first, last + 1):
        img = Image.new("L", (cell_w, cell_h), 0)
        draw = ImageDraw.Draw(img)
        draw.text((0, 0), chr(c), font=font, fill=255)
        for y in range(cell_h):
            row = bytearray(stride)
            for x in range(cell_w):
                if img.getpixel((x, y)) >= 128:
                    row[x >> 3] |= 0x80 >> (x & 7)
            bitmap.extend(row)

    count = last - first + 1
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
    if len(sys.argv) != 4:
        sys.exit(f"usage: {sys.argv[0]} <font> <size_px> <symbol>")
    print(convert(sys.argv[1], int(sys.argv[2]), sys.argv[3]))
