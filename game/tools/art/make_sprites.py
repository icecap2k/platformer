#!/usr/bin/env python3
"""Draws game/assets/sprites/sprites.png: coins, HUD hearts and the healing fruit (standard library only).

    python3 game/tools/art/make_sprites.py [--preview preview.png]

Layout of the sheet (the renderer reads these rows): the coin frames at y = 18 are copied from
the sheet already on disk, so running this script never touches them. The HUD hearts (full,
empty) are in the row below the coins (y = 26) and the healing fruit frames (16x16 with a glow
halo) below them (y = 34). Rows 0-17 are empty: the hero lives in its own sheet
(make_hero_sprites.py).

The other art scripts import `write_png` from here.
"""

import struct
import sys
import zlib
from pathlib import Path

SHEET_WIDTH = 204
SHEET_PATH = Path(__file__).resolve().parents[2] / "assets" / "sprites" / "sprites.png"
COIN_ROWS = (18, 26)  # y range of the coin frames in the sheet

OUTLINE = (28, 20, 32, 255)


# --------------------------------------------------------------------------- PNG

def read_png(path):
    data = path.read_bytes()
    assert data[:8] == b"\x89PNG\r\n\x1a\n"
    pos, idat = 8, b""
    width = height = 0
    while pos < len(data):
        length, kind = struct.unpack(">I4s", data[pos:pos + 8])
        chunk = data[pos + 8:pos + 8 + length]
        if kind == b"IHDR":
            width, height, depth, color = struct.unpack(">IIBB", chunk[:10])
            assert depth == 8 and color == 6, "expected an 8-bit RGBA PNG"
        elif kind == b"IDAT":
            idat += chunk
        pos += 12 + length
    raw, stride = zlib.decompress(idat), width * 4
    rows, previous = [], bytearray(stride)
    for y in range(height):
        filter_type = raw[y * (stride + 1)]
        line = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for i in range(stride):
            a = line[i - 4] if i >= 4 else 0
            b = previous[i]
            c = previous[i - 4] if i >= 4 else 0
            if filter_type == 1:
                line[i] = (line[i] + a) & 255
            elif filter_type == 2:
                line[i] = (line[i] + b) & 255
            elif filter_type == 3:
                line[i] = (line[i] + (a + b) // 2) & 255
            elif filter_type == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pred = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                line[i] = (line[i] + pred) & 255
        rows.append(bytes(line))
        previous = line
    pixels = [[tuple(row[x * 4:x * 4 + 4]) for x in range(width)] for row in rows]
    return width, height, pixels


def write_png(path, pixels):
    height, width = len(pixels), len(pixels[0])
    raw = b"".join(b"\x00" + bytes(c for px in row for c in px) for row in pixels)

    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))

    png = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)) +
           chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))
    path.write_bytes(png)


# ------------------------------------------------------------------------ drawing

# Heart icons for the HUD: 8x8, full then empty, in the row below the coins.
HEART_ROW = (26, 34)
HEART = [
    "........",
    ".OO..OO.",
    "OhrOOrrO",
    "OhrrrrrO",
    "OrrrrrdO",
    ".OrrrdO.",
    "..OrdO..",
    "...OO...",
]
# Healing fruit (a glowing cave berry): 4 frames of 16x16 in the rows below the hearts. The
# berry (8x8) sits in the middle of a warm, semi-transparent HALO that breathes (it grows
# and brightens, then shrinks) with twinkling sparkles around it, so it reads as glowing
# even on a dark cave background.
FRUIT_ROW = (34, 50)
FRUIT_CELL = 16
FRUIT_PALETTE = {"a": (255, 218, 176, 255), "b": (255, 120, 96, 255), "d": (204, 58, 92, 255),
                 "g": (110, 214, 128, 255)}
FRUIT_BODY = [
    "...gg...",
    "..OOgO..",
    ".OaabbO.",
    "OaabbbdO",
    "ObbbbbdO",
    "ObbbbddO",
    ".OddddO.",
    "..OOOO..",
]
FRUIT_HALO = [        # (radius in px, peak alpha) per frame: it breathes
    (6.0, 110),
    (7.2, 165),
    (8.0, 215),
    (7.2, 165),
]
FRUIT_SPARKLES = [    # (x, y) of the twinkles per frame
    [(2, 3), (13, 12)],
    [(13, 3), (2, 12), (8, 1)],
    [(3, 14), (12, 2), (1, 7), (14, 9)],
    [(8, 14), (1, 2), (14, 6)],
]
HALO_COLOUR = (255, 214, 150)
SPARKLE_COLOUR = (255, 252, 214)


def fruit_frame(index):
    """A 16x16 grid of RGBA pixels: halo, sparkles and the berry on top."""
    import math
    cell = [[(0, 0, 0, 0)] * FRUIT_CELL for _ in range(FRUIT_CELL)]
    radius, peak = FRUIT_HALO[index]
    for y in range(FRUIT_CELL):
        for x in range(FRUIT_CELL):
            distance = math.hypot(x + 0.5 - FRUIT_CELL / 2, y + 0.5 - FRUIT_CELL / 2)
            if distance < radius:
                alpha = round(peak * (1.0 - distance / radius) ** 1.2)
                if alpha > 0:
                    cell[y][x] = HALO_COLOUR + (alpha,)
    for cx, cy in FRUIT_SPARKLES[index]:            # a small plus, bright in the middle
        for dx, dy, alpha in ((0, 0, 255), (1, 0, 150), (-1, 0, 150), (0, 1, 150), (0, -1, 150)):
            x, y = cx + dx, cy + dy
            if 0 <= x < FRUIT_CELL and 0 <= y < FRUIT_CELL:
                cell[y][x] = SPARKLE_COLOUR + (alpha,)
    offset = (FRUIT_CELL - 8) // 2
    for y, row in enumerate(FRUIT_BODY):
        for x, char in enumerate(row):
            if char != ".":
                cell[offset + y][offset + x] = OUTLINE if char == "O" else FRUIT_PALETTE[char]
    return cell


HEART_FULL = {"r": (222, 60, 76, 255), "h": (255, 160, 160, 255), "d": (150, 30, 60, 255)}
HEART_EMPTY = {"r": (60, 48, 72, 255), "h": (88, 74, 102, 255), "d": (44, 34, 56, 255)}


def build_sheet():
    coin_top, coin_bottom = COIN_ROWS
    old_w, old_h, old = read_png(SHEET_PATH)
    assert old_h >= coin_bottom, "the sheet on disk has no coin rows"
    width, height = SHEET_WIDTH, FRUIT_ROW[1]
    sheet = [[(0, 0, 0, 0)] * width for _ in range(height)]
    for y in range(coin_top, coin_bottom):          # keep the coins as they are
        for x in range(min(old_w, width)):
            sheet[y][x] = old[y][x]
    for index, palette in enumerate((HEART_FULL, HEART_EMPTY)):
        for y, row in enumerate(HEART):
            for x, char in enumerate(row):
                if char != ".":
                    sheet[HEART_ROW[0] + y][index * 8 + x] = OUTLINE if char == "O" else palette[char]
    for index in range(len(FRUIT_HALO)):
        frame_pixels = fruit_frame(index)
        for y in range(FRUIT_CELL):
            for x in range(FRUIT_CELL):
                sheet[FRUIT_ROW[0] + y][index * FRUIT_CELL + x] = frame_pixels[y][x]
    return sheet


def write_preview(path, sheet, scale=10):
    background = (120, 170, 200, 255)
    rows = []
    for row in sheet:
        scaled = []
        for px in row:
            scaled.extend([px if px[3] else background] * scale)
        rows.extend([scaled] * scale)
    write_png(path, rows)


if __name__ == "__main__":
    sheet = build_sheet()
    if "--preview" in sys.argv:
        write_preview(Path(sys.argv[sys.argv.index("--preview") + 1]), sheet)
    else:
        write_png(SHEET_PATH, sheet)
        print(f"wrote {SHEET_PATH}")
