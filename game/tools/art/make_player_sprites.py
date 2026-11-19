#!/usr/bin/env python3
"""Draws the player frames of game/assets/sprites/sprites.png (standard library only).

    python3 game/tools/art/make_player_sprites.py [--preview preview.png]

Layout of the sheet: one 12x18 cell per player frame, left to right, in the
order of `presentation::PlayerPose` (see src/presentation/animation.h). The coin
frames below them (y = 18) are copied from the sheet already on disk, so running
this script never touches the coins. The HUD hearts (full, empty) are drawn in
the row below the coins (y = 26).

Frames face right; the renderer mirrors them to face left. The collision box is
8x16 and sits at cell (2, 1): the figure is drawn inside x 1..10, y 1..16 and a
1 px dark outline is added around it automatically.
"""

import struct
import sys
import zlib
from pathlib import Path

CELL_W, CELL_H = 12, 18
SHEET_PATH = Path(__file__).resolve().parents[2] / "assets" / "sprites" / "sprites.png"
COIN_ROWS = (18, 26)  # y range of the coin frames in the sheet

# Palette
OUTLINE = (28, 20, 32, 255)
GREEN = (40, 108, 72, 255)          # tunic and hood
GREEN_LIGHT = (72, 150, 100, 255)
GREEN_DARK = (26, 74, 56, 255)
SHADOW = (12, 28, 26, 255)          # inside the hood: the eyes are hidden here
SKIN = (255, 204, 170, 255)
SKIN_SHADE = (226, 156, 132, 255)
BELT = (122, 84, 52, 255)
BUCKLE = (236, 196, 84, 255)
BOOT = (104, 66, 48, 255)
BOOT_LIGHT = (150, 100, 68, 255)    # cuff
SOLE = (52, 36, 36, 255)


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

class Cell:
    def __init__(self):
        self.px = {}

    def put(self, x, y, color):
        if 1 <= x <= 10 and 1 <= y <= 16:  # the figure stays inside the outline margin
            self.px[(x, y)] = color

    def rect(self, x0, y0, x1, y1, color):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.put(x, y, color)

    def outlined(self):
        out = dict(self.px)
        for (x, y) in self.px:
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                nx, ny = x + dx, y + dy
                if (nx, ny) not in self.px and 0 <= nx < CELL_W and 0 <= ny < CELL_H:
                    out[(nx, ny)] = OUTLINE
        return out


def draw_hood_tip(cell, head_y, tail):
    """The pointed tip of the hood streams behind the head. `tail` is a list of
    (dx, dy) relative to the head's top-left corner (3, head_y)."""
    for i, (dx, dy) in enumerate(tail):
        color = GREEN_LIGHT if i == len(tail) - 1 else (GREEN_DARK if i % 2 else GREEN)
        cell.put(3 + dx, head_y + dy, color)


def draw_head(cell, y):
    """A hood that hides the eyes; only the nose, mouth and chin show."""
    cell.rect(4, y, 7, y, GREEN)                  # top of the hood
    cell.put(5, y, GREEN_LIGHT)
    cell.put(6, y, GREEN_LIGHT)
    cell.rect(3, y + 1, 8, y + 1, GREEN)          # brim
    cell.put(4, y + 1, GREEN_LIGHT)
    cell.rect(3, y + 2, 4, y + 3, GREEN)          # hood behind the face
    cell.rect(5, y + 2, 8, y + 2, SHADOW)         # the eyes, in shadow
    cell.put(5, y + 3, SHADOW)
    cell.rect(6, y + 3, 8, y + 3, SKIN)           # nose and mouth
    cell.put(6, y + 3, SKIN_SHADE)
    cell.rect(3, y + 4, 5, y + 4, GREEN_DARK)     # hood draping over the shoulder
    cell.rect(6, y + 4, 7, y + 4, SKIN)           # chin


def draw_torso(cell, y):
    cell.rect(4, y, 7, y, GREEN_LIGHT)            # cowl
    cell.rect(4, y + 1, 7, y + 2, GREEN)
    cell.put(4, y + 1, GREEN_LIGHT)
    cell.rect(4, y + 3, 7, y + 3, BELT)           # belt
    cell.put(6, y + 3, BUCKLE)
    cell.rect(4, y + 4, 7, y + 4, GREEN)
    cell.rect(7, y + 1, 7, y + 2, GREEN_DARK)
    cell.rect(3, y + 5, 8, y + 5, GREEN_DARK)     # the tunic's hem flares out


def draw_arm(cell, side, mode, y, color):
    """`side` is 'back' (x = 3) or 'front' (x = 8). Sleeves are green, hands skin."""
    sx = 3 if side == "back" else 8
    sy = y + 1
    out = -1 if side == "back" else 1  # direction away from the body
    if mode == "down":
        pixels = [(sx, sy), (sx, sy + 1), (sx, sy + 2, "hand")]
    elif mode == "forward":
        pixels = [(sx, sy), (sx + out, sy + 1), (sx + out, sy + 2, "hand")]
    elif mode == "backward":
        pixels = [(sx, sy), (sx - out, sy + 1), (sx - out, sy + 2, "hand")]
    elif mode == "up":
        pixels = [(sx + out, sy + 1), (sx + out, sy), (sx + out, sy - 1, "hand")]
    elif mode == "out":
        pixels = [(sx, sy), (sx + out, sy), (sx + 2 * out, sy - 1, "hand")]
    else:
        raise ValueError(mode)
    for p in pixels:
        cell.put(p[0], p[1], SKIN if len(p) == 3 else color)


def draw_leg(cell, hip_x, dx, lift, skin_column):
    """A 1 px wide bare leg and a 2 px wide boot, from just under the tunic (y = 12) down to
    a foot that is `dx` px forward and `lift` px above the ground."""
    top, bottom = 12, 16 - lift
    span = max(1, bottom - top)
    for y in range(top, bottom + 1):
        x = hip_x + round(dx * (y - top) / span)
        if y == bottom:
            color = SOLE
        elif y == bottom - 1:
            color = BOOT
        elif y == bottom - 2:
            color = BOOT_LIGHT                    # boot cuff
        else:
            cell.put(x + skin_column, y, SKIN)    # bare leg, one pixel wide
            continue
        cell.rect(x, y, x + 1, y, color)


def frame(drop=0, tail=(), legs=((0, 0), (0, 0)), arms=("down", "down"), head_drop=None):
    """`drop` lowers the whole body, `head_drop` only the head (default: same).
    legs = ((dx, lift) of the back leg, (dx, lift) of the front leg)."""
    cell = Cell()
    head_y = 1 + (drop if head_drop is None else head_drop)
    body_y = 1 + drop
    draw_hood_tip(cell, head_y, tail)
    draw_arm(cell, "back", arms[0], body_y + 5, GREEN_DARK)
    draw_leg(cell, 4, *legs[0], skin_column=0)
    draw_leg(cell, 6, *legs[1], skin_column=1)
    draw_torso(cell, body_y + 5)
    draw_head(cell, head_y)
    draw_arm(cell, "front", arms[1], body_y + 5, GREEN)
    return cell


# ------------------------------------------------------------------------- frames
# Order = presentation::PlayerPose.

STANCE = ((-1, 0), (1, 0))  # feet slightly apart so the legs do not merge
IDLE_TAIL = [(-1, 2), (-1, 3), (-1, 4), (-1, 5)]
def crouch_frame(tail=((-1, 2), (-2, 2), (-2, 3))):
    """Crouching: the collision box is half as tall (8 px), so the figure is squat: the
    hood, a short body and the boots, with the hands resting on the knees. Its feet are
    on the same row as in every other frame (the sprite is bottom aligned)."""
    cell = Cell()
    head_y = 8
    draw_hood_tip(cell, head_y, list(tail))
    cell.rect(2, 13, 2, 13, GREEN_DARK)           # back arm, resting on the knee
    cell.put(2, 14, SKIN)
    cell.rect(4, 13, 7, 13, GREEN_LIGHT)          # cowl
    cell.rect(3, 14, 8, 14, GREEN_DARK)           # the hem, flared
    cell.rect(4, 14, 7, 14, BELT)
    cell.put(6, 14, BUCKLE)
    for x0, x1 in ((3, 5), (6, 8)):               # two boots, wide apart
        cell.rect(x0, 15, x1, 15, BOOT)
        cell.rect(x0, 16, x1, 16, SOLE)
    cell.put(3, 15, BOOT_LIGHT)
    cell.put(8, 15, BOOT_LIGHT)
    draw_head(cell, head_y)
    cell.put(9, 13, GREEN)                        # front arm
    cell.put(9, 14, SKIN)
    return cell


FRAMES = [
    # Idle: breathing (the head sinks one pixel) while the hood's tip sways.
    frame(0, IDLE_TAIL, legs=STANCE),
    frame(0, [(-1, 2), (-1, 3), (-1, 4), (-2, 5)], legs=STANCE),
    frame(0, [(-1, 2), (-1, 3), (-2, 4), (-2, 5)], legs=STANCE, head_drop=1),
    frame(0, [(-1, 2), (-1, 3), (-1, 4), (-2, 5)], legs=STANCE, head_drop=1),
    # Run: two legs, six poses. A forward -> passing -> B forward -> passing.
    frame(0, [(-1, 2), (-2, 2), (-2, 3)], legs=((-2, 0), (2, 0)), arms=("forward", "backward"), head_drop=1),
    frame(0, [(-1, 2), (-2, 2), (-2, 3)], legs=((-2, 2), (1, 0)), arms=("down", "down"), head_drop=1),
    frame(0, [(-1, 2), (-2, 2), (-2, 1)], legs=((1, 3), (0, 0)), arms=("backward", "forward")),
    frame(0, [(-1, 2), (-2, 2), (-2, 3)], legs=((2, 0), (-2, 0)), arms=("backward", "forward"), head_drop=1),
    frame(0, [(-1, 2), (-2, 2), (-2, 3)], legs=((1, 0), (-2, 2)), arms=("down", "down"), head_drop=1),
    frame(0, [(-1, 2), (-2, 2), (-2, 1)], legs=((0, 0), (1, 3)), arms=("forward", "backward")),
    # Rise, apex, fall: the hood's tip trails behind the movement.
    frame(0, [(-1, 2), (-1, 3), (-1, 4), (-1, 5), (-1, 6)], legs=((-1, 2), (1, 3)), arms=("up", "up")),
    frame(0, [(-1, 2), (-2, 2), (-2, 1)], legs=((-1, 1), (1, 2)), arms=("out", "out")),
    frame(0, [(-1, 2), (-1, 1), (-1, 0), (-1, -1)], legs=((-1, 0), (1, 1)), arms=("up", "up"), head_drop=1),
    # Land: a squashed crouch.
    frame(2, [(-1, 3), (-2, 4), (-2, 5)], legs=((-1, 0), (1, 0)), arms=("forward", "forward")),
    # Hurt: thrown back, arms flailing, the hood's tip flung up.
    frame(0, [(-1, 2), (-2, 1), (-2, 0), (-2, -1)], legs=((-1, 1), (2, 0)), arms=("up", "backward"), head_drop=1),
    # Slide: crouched and leaning back, one leg stretched out in front, arms out for balance.
    frame(2, [(-1, 4), (-2, 4), (-2, 5)], legs=((-1, 0), (3, 0)), arms=("backward", "forward"), head_drop=3),
    # Crouch: half the height, the hitbox is 8 px tall.
    crouch_frame(),
]

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
HEART_FULL = {"r": (222, 60, 76, 255), "h": (255, 160, 160, 255), "d": (150, 30, 60, 255)}
HEART_EMPTY = {"r": (60, 48, 72, 255), "h": (88, 74, 102, 255), "d": (44, 34, 56, 255)}


def build_sheet():
    coin_top, coin_bottom = COIN_ROWS
    old_w, old_h, old = read_png(SHEET_PATH)
    assert old_h >= coin_bottom, "the sheet on disk has no coin rows"
    width, height = CELL_W * len(FRAMES), HEART_ROW[1]
    sheet = [[(0, 0, 0, 0)] * width for _ in range(height)]
    for index, cell in enumerate(FRAMES):
        for (x, y), color in cell.outlined().items():
            sheet[y][index * CELL_W + x] = color
    for y in range(coin_top, coin_bottom):          # keep the coins as they are
        for x in range(min(old_w, width)):
            sheet[y][x] = old[y][x]
    for index, palette in enumerate((HEART_FULL, HEART_EMPTY)):
        for y, row in enumerate(HEART):
            for x, char in enumerate(row):
                if char != ".":
                    sheet[HEART_ROW[0] + y][index * 8 + x] = OUTLINE if char == "O" else palette[char]
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
        print(f"wrote {SHEET_PATH} ({len(FRAMES)} player frames)")
