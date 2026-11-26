#!/usr/bin/env python3
"""Draws the heroine of game/assets/sprites/hero.png (standard library only).

    python3 game/tools/art/make_hero_sprites.py
    python3 game/tools/art/make_hero_sprites.py --preview preview.png   # every pose, 8x enlarged

A grid of 32x28 cells: one column per `presentation::PlayerPose`, in that order, and one row per
flutter frame (4) of her long hair.

A small chunky pixel figure, drawn after a reference picture by the author: long white hair with a
red clip, simple dark eyes and no mouth, a black jacket over a grey t-shirt, red trousers and brown
boots, with a thick dark outline. The head is hand drawn; arms, legs and the flowing lock of hair
are placed per pose. Frames face right; the renderer mirrors them. The figure is centred in the
cell, its soles are on row 26. The collision box stays 8x16.
"""

import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_sprites import write_png  # noqa: E402

CELL_W, CELL_H = 32, 28
SOLE_Y = 26
FLUTTER_FRAMES = 4
OUT = Path(__file__).resolve().parents[2] / "assets" / "sprites" / "hero.png"

PALETTE = {
    "O": (6, 4, 8, 255),           # outline
    "W": (255, 255, 255, 255),     # hair, glint
    "H": (231, 235, 245, 255),     # hair
    "h": (162, 171, 195, 255),     # hair, shade
    "r": (191, 28, 30, 255),       # hair clip
    "S": (250, 198, 156, 255),     # skin
    "s": (225, 145, 122, 255),     # skin, shade
    "n": (206, 119, 99, 255),      # neck
    "e": (37, 37, 46, 255),        # eyes
    "J": (50, 50, 52, 255),        # jacket
    "j": (34, 34, 38, 255),        # jacket, far sleeve
    "d": (42, 46, 46, 255),        # t-shirt, shaded
    "G": (82, 86, 86, 255),        # t-shirt
    "P": (156, 36, 45, 255),       # trousers
    "q": (104, 20, 42, 255),       # trousers, shade
    "B": (106, 65, 42, 255),       # boots
    "b": (78, 46, 34, 255),        # boots, far one
}


class Cell:
    def __init__(self):
        self.px = {}

    def paste(self, shape, outline=False):
        """Puts a part (a dict of pixels) on the figure, optionally separated from what is
        already there by its own outline."""
        def fits(x, y):
            return 1 <= x <= CELL_W - 2 and 1 <= y <= SOLE_Y
        if outline:
            for (x, y) in shape:
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    n = (x + dx, y + dy)
                    if n not in shape and n in self.px and fits(*n):
                        self.px[n] = "O"
        for (x, y), key in shape.items():
            if fits(x, y):
                self.px[(x, y)] = key

    def art(self, x, y, rows):
        """Paints rows of palette letters with the top left corner at (x, y); '.' is empty."""
        self.paste({(x + dx, y + dy): key for dy, row in enumerate(rows)
                    for dx, key in enumerate(row) if key != "."})

    def outlined(self):
        out = dict(self.px)
        for (x, y) in self.px:
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                n = (x + dx, y + dy)
                if n not in self.px and 0 <= n[0] < CELL_W and 0 <= n[1] < CELL_H:
                    out[n] = "O"
        return out


# The head, 12 x 9 (its first row is left for the outline), copied from the reference.
HEAD = [
    "............",
    "....HWHHH...",
    "...HrHHHHH..",
    "..HHHrhHhH..",
    "..HHHseseh..",
    "..HhHSeSeh..",
    "..HhWSSSSH..",
    ".HHhHSSSSH..",
    ".HhOhHnnOhH.",
]

# --------------------------------------------------------------------------- hair
# For each pose: where the long lock ends (relative to where it leaves the back of the head),
# how much it waves and how it bends.
HAIR = {
    "rest": ((-4.0, 5.5), 0.45, -0.9),
    "run": ((-6.0, 2.0), 0.7, -0.5),
    "rise": ((-3.0, 6.5), 0.5, -0.8),
    "float": ((-6.0, 0.0), 0.6, 0.6),
    "fall": ((-4.0, -4.0), 0.6, 0.7),
    "slide": ((-6.0, 1.5), 0.5, -0.5),
    "crouch": ((-5.0, 3.5), 0.25, -0.6),
}


def flowing_hair(cell, x, y, style, flutter):
    """A tapering lock that grows out of the back of the head and waves: lit on top, shaded below."""
    (ex, ey), wave, bend = HAIR[style]
    phase = flutter * 2.0 * math.pi / FLUTTER_FRAMES
    length = math.hypot(ex, ey) or 1.0
    nx, ny = -ey / length, ex / length
    shape = {}
    steps = 24
    for i in range(steps + 1):
        s = i / steps
        side = wave * s * math.sin(phase - s * 3.5) + bend * math.sin(math.pi * s)
        px, py = x + ex * s + nx * side, y + ey * s + ny * side
        radius = 1.6 * (1.0 - s) ** 0.6 + 0.65
        r = int(math.ceil(radius))
        for dy in range(-r, r + 1):
            for dx in range(-r, r + 1):
                if dx * dx + dy * dy <= radius * radius:
                    shape[(round(px) + dx, round(py) + dy)] = "h" if dy > radius * 0.3 else "H"
    cell.paste(shape)


# --------------------------------------------------------------------------- legs
# Hand drawn, 10 x 5, with their top left corner two columns into the figure and the boots on the
# bottom row. 'P'/'B' = the leg nearer to us, 'q'/'b' = the farther one. Knees bend and feet never
# stray far from the hips, so the stride reads as steps and not as two sticks swinging.

LEGS_STAND = [
    "....qq....",
    "..PPqqP...",
    "..PP.qP...",
    "..q...q...",
    "..B...B...",
]
# Half of the run cycle (the near leg leads); the other half is the same with the legs swapped.
RUN_HALF = [
    [   # contact: the near foot lands ahead
        "...qPPP...",
        "...qqPP...",
        "..qq..PP..",
        "..q....P..",
        "..b....BB.",
    ],
    [   # down: the weight sinks over it, the far foot lifts behind
        "...qPPP...",
        "..qqqPP...",
        "..bq.PP...",
        "..b..PP...",
        ".....BB...",
    ],
    [   # passing: the far leg comes through, knee first
        "...qPPP...",
        "...qqPPq..",
        "....PPbq..",
        "....PP....",
        "....BB....",
    ],
    [   # up: pushing off, the far knee ahead
        "...PPqq...",
        "...PP.qq..",
        "..PP...q..",
        "..P....bb.",
        "..BB......",
    ],
]
SWAP = str.maketrans("PqBb", "qPbB")
RUN_LEGS = RUN_HALF + [[row.translate(SWAP) for row in frame] for frame in RUN_HALF]

LEGS_RISE = [   # tucked on the way up
    "...qqPP...",
    "..qq..PP..",
    "..qb.PP...",
    ".....BB...",
    "..........",
]
LEGS_FLOAT = [
    "...qqPP...",
    "..qq..PP..",
    "..bb..BB..",
    "..........",
    "..........",
]
LEGS_FALL = [   # reaching for the ground
    "...qqPP...",
    "...q..PP..",
    "..qq...P..",
    "..q....BB.",
    "..b.......",
]
LEGS_LAND = [
    "...qqPP...",
    "...qqPP...",
    "..qq..PP..",
    "..q....P..",
    ".bb....BB.",
]
LEGS_SLIDE = [  # sitting back, the near leg stretched ahead
    "...qqPP...",
    "...qqPP...",
    "...qqPPP..",
    "..qq..PPPP",
    "..bb....BB",
]


def legs_at(cell, x, art):
    cell.art(x + 2, SOLE_Y - len(art) + 1, art)


# --------------------------------------------------------------------------- arms

def arm(cell, x, y, swing, lift, front):
    """A short sleeve from the shoulder (x, y) to a hand. At rest the hand hangs 3 px below;
    `swing` moves it forward (+) or back (-), `lift` raises it (negative = up)."""
    hand_x, hand_y = x + swing, y + 3 + lift
    steps = max(1, hand_y - y)
    key = "J" if front else "j"
    shape = {}
    for i in range(steps):
        at = x + int(swing * i / steps)          # the sleeve leans towards the hand
        row = y + round((hand_y - y) * i / steps)
        shape[(at, row)] = shape[(at + 1, row)] = key
    skin = "S" if front else "s"
    shape[(hand_x, hand_y)] = shape[(hand_x + 1, hand_y)] = skin
    cell.paste(shape, outline=True)


# --------------------------------------------------------------------------- body

def body(style="rest", legs=LEGS_STAND, drop=0, head_drop=None, lean=0, back=(0, 0), front=(0, 0)):
    """Returns a function that draws the pose with a given flutter frame. `back` and `front` are
    each arm's (swing, lift)."""
    def draw(flutter):
        cell = Cell()
        left = 10 + lean                       # column 0 of the reference
        head_y = 10 + (drop if head_drop is None else head_drop)
        torso_y = 19 + drop
        flowing_hair(cell, left + 3.0, head_y + 5.0, style, flutter)
        legs_at(cell, 10, legs)
        arm(cell, 10 + (lean // 2) + 8, torso_y, *back, front=False)
        # the jacket, open over the t-shirt, down to the hips
        for row in range(torso_y, min(torso_y + 3, SOLE_Y - 2)):
            cell.art(10 + (lean // 2) + 3, row, ["JJdGJJ"])
        arm(cell, 10 + (lean // 2) + 2, torso_y, *front, front=True)
        cell.art(left, head_y, HEAD)
        cell.art(left + 5, head_y + 9, ["H"])
        return cell
    return draw


def crouch(flutter):
    """The box is 8 px tall while crouching: her head rests just over her boots."""
    cell = Cell()
    flowing_hair(cell, 13.0, 20.0, "crouch", flutter)
    legs_at(cell, 11, ["..qqPP....", "..bb..BB.."])
    cell.art(13, 23, ["JJdGJJ", "qqqqPP"])
    cell.art(10, 15, HEAD)
    arm(cell, 18, 22, 1, -1, front=True)
    return cell


def run_pose(index):
    """One of the 8 frames of the run: the body sinks on each landing and rises on the push, and
    each arm swings against the leg on its side."""
    step = index % 4
    drop = (0, 1, 0, 0)[step]
    swing = (-2, -1, 0, 1, 2, 1, 0, -1)[index]  # the near arm: back when the near leg leads
    lift = -1 if abs(swing) == 2 else 0
    return body("run", legs=RUN_LEGS[index], drop=drop, head_drop=drop, lean=1,
                back=(-swing, lift), front=(swing, lift))


# Order = presentation::PlayerPose.
POSES = [
    # Idle: breathing.
    body(),
    body(),
    body(head_drop=1),
    body(head_drop=1),
    # Run: contact, down, passing, up; then the same on the other leg.
    *[run_pose(index) for index in range(8)],
    # Rise, apex, fall: the arms open a little, never straight out.
    body("rise", LEGS_RISE, lean=1, back=(1, -2), front=(-1, -1)),
    body("float", LEGS_FLOAT, back=(1, -2), front=(-2, -2)),
    body("fall", LEGS_FALL, head_drop=1, back=(2, -2), front=(-2, -2)),
    # Land.
    body("rest", LEGS_LAND, drop=1, head_drop=2, back=(1, -1), front=(-1, -1)),
    # Hurt.
    body("fall", LEGS_FALL, lean=-1, back=(2, -3), front=(-2, -2)),
    # Slide.
    body("slide", LEGS_SLIDE, drop=2, lean=-1, back=(1, -1), front=(-2, -1)),
    crouch,
]


def render():
    sheet = [[(0, 0, 0, 0)] * (len(POSES) * CELL_W) for _ in range(FLUTTER_FRAMES * CELL_H)]
    for column, pose in enumerate(POSES):
        for flutter in range(FLUTTER_FRAMES):
            for (x, y), key in pose(flutter).outlined().items():
                sheet[flutter * CELL_H + y][column * CELL_W + x] = PALETTE[key]
    return sheet


def write_preview(path, scale=8):
    """Every pose (flutter frame = pose index mod 4), six per line, enlarged."""
    sheet = render()
    background = (14, 13, 32, 255)
    per_row = 6
    rows = (len(POSES) + per_row - 1) // per_row
    canvas = [[background] * (per_row * CELL_W) for _ in range(rows * CELL_H)]
    for i in range(len(POSES)):
        ox, oy = (i % per_row) * CELL_W, (i // per_row) * CELL_H
        for y in range(CELL_H):
            for x in range(CELL_W):
                px = sheet[(i % FLUTTER_FRAMES) * CELL_H + y][i * CELL_W + x]
                if px[3]:
                    canvas[oy + y][ox + x] = px
    big = []
    for line in canvas:
        wide = [px for px in line for _ in range(scale)]
        big.extend([wide] * scale)
    write_png(path, big)


if __name__ == "__main__":
    if "--preview" in sys.argv:
        write_preview(Path(sys.argv[sys.argv.index("--preview") + 1]))
    else:
        write_png(OUT, render())
        print(f"wrote {OUT} ({len(POSES)} poses x {FLUTTER_FRAMES} flutter frames)")
