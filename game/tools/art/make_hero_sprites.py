#!/usr/bin/env python3
"""Draws the hero of game/assets/sprites/hero.png (standard library only).

    python3 game/tools/art/make_hero_sprites.py
    python3 game/tools/art/make_hero_sprites.py --preview preview.png   # every pose, 8x enlarged

A grid of 32x28 cells: one column per `presentation::PlayerPose`, in that order, and one row per
flutter frame (4) of the tip of the hood.

A small chunky figure: a big hood that is most of its height and flows into a short mantle, its
front hanging low over a face in shadow (no eyes, hair or mouth, only the nose and chin), stubby arms, short legs, big boots
and a satchel on its back. The hood, its tip and the body are painted into ONE shape and
outlined once, so the hood has no seams. Frames face right; the renderer mirrors them. The
figure is centred in the cell, its soles are on row 26. The collision box stays 8x16.
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
    "O": (22, 12, 22, 255),        # outline
    "L": (134, 224, 255, 255),     # cloth, lit
    "M": (58, 156, 240, 255),      # cloth: a bright blue, to stand out on the dark blue cave
    "D": (38, 102, 196, 255),      # cloth, shade
    "X": (34, 60, 130, 255),       # cloth, deepest (folds, rim)
    "V": (22, 22, 50, 255),        # the shadow inside the hood
    "S": (244, 198, 154, 255),     # skin
    "s": (212, 148, 104, 255),     # skin, shade
    "B": (104, 62, 74, 255),       # leggings
    "b": (72, 42, 58, 255),        # leggings, back leg
    "T": (150, 86, 52, 255),       # boots
    "t": (104, 58, 44, 255),       # boots, back one
    "C": (244, 168, 58, 255),      # satchel (a warm accent)
    "c": (184, 108, 40, 255),
    "g": (255, 238, 160, 255),     # buckle
}


class Cell:
    def __init__(self):
        self.px = {}

    def put(self, x, y, key):
        if 1 <= x <= CELL_W - 2 and 1 <= y <= SOLE_Y:  # room for the outline
            self.px[(x, y)] = key

    def rect(self, x0, y0, x1, y1, key):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.put(x, y, key)

    def art(self, x, y, rows):
        """Paints rows of palette letters with the top-left corner at (x, y); '.' is empty."""
        for dy, row in enumerate(rows):
            for dx, key in enumerate(row):
                if key != ".":
                    self.put(x + dx, y + dy, key)

    def outlined(self):
        out = dict(self.px)
        for (x, y) in self.px:
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                n = (x + dx, y + dy)
                if n not in self.px and 0 <= n[0] < CELL_W and 0 <= n[1] < CELL_H:
                    out[n] = "O"
        return out


# The hood and face, 10 x 10, hand drawn. Its front hangs low over the face, so only the tip
# of the nose and the chin show; it ends in the mantle that covers the shoulders.
HEAD = [
    "....LLLL..",
    "..LLLLLLM.",
    ".LLMMMMMM.",
    ".LMMMMMMMM",
    ".MMDXXXXMM",
    "MMDXVVVXMM",
    "MMDXVVVVXM",
    "MDDXsSSSVX",
    ".DDMXsSX..",
    "..DMMXX...",
]

# ------------------------------------------------------------------- the hood's tip
# For each pose: where the tip ends (relative to where it leaves the back of the hood),
# how much it waves and how it bends.
TIP = {
    "rest": ((-4, 6), 0.7, -1.2),
    "run": ((-8, 1), 1.5, -1.0),
    "rise": ((-5, 7), 0.9, -1.5),
    "float": ((-8, -1), 1.2, 1.0),
    "fall": ((-5, -6), 1.4, 1.2),
    "slide": ((-8, 1), 1.0, -1.0),
    "crouch": ((-6, 3), 0.4, -1.0),
}


def draw_tip(cell, x, y, style, flutter):
    """A fat, tapering point that grows out of the back of the hood whose top-left is (x, y)."""
    (ex, ey), wave, bend = TIP[style]
    ax, ay = x + 3, y + 4                      # inside the hood, so there is no gap
    phase = flutter * 2.0 * math.pi / FLUTTER_FRAMES
    length = math.hypot(ex, ey) or 1.0
    nx, ny = -ey / length, ex / length
    painted = {}
    steps = 24
    for i in range(steps + 1):
        s = i / steps
        side = wave * s * math.sin(phase - s * 3.5) + bend * math.sin(math.pi * s)
        px, py = ax + ex * s + nx * side, ay + ey * s + ny * side
        radius = 2.3 * (1.0 - s) ** 0.8 + 0.4
        r = int(math.ceil(radius))
        for dy in range(-r, r + 1):
            for dx in range(-r, r + 1):
                if dx * dx + dy * dy <= radius * radius:
                    shade = "L" if dy < -radius * 0.4 else ("D" if dy > radius * 0.35 else "M")
                    painted[(round(px) + dx, round(py) + dy)] = shade
    for (px, py), key in painted.items():
        cell.put(px, py, key)


# ------------------------------------------------------------------------ the body

def draw_leg(cell, hip_x, hip_y, dx, lift, front):
    """A short 2 px leg and a chunky boot; the foot is `dx` px forward and `lift` px up."""
    boot_top = SOLE_Y - 1 - lift
    span = max(1, boot_top - hip_y)
    for y in range(hip_y, boot_top):
        x = hip_x + round(dx * (y - hip_y) / span)
        cell.rect(x, y, x + 1, y, "B" if front else "b")
    x = hip_x + dx
    cell.rect(x, boot_top, x + 1, boot_top, "T" if front else "t")
    cell.rect(x, boot_top + 1, x + 2, boot_top + 1, "T" if front else "t")


def draw_arm(cell, x, y, mode, front):
    """A stubby sleeve out of the mantle with a 2 px hand, from the shoulder (x, y)."""
    paths = {  # sleeve pixels, then the hand
        "down": ([(0, 0), (0, 1)], (0, 2)),
        "forward": ([(0, 0), (1, 1), (2, 1)], (3, 1)),
        "backward": ([(0, 0), (-1, 1), (-2, 1)], (-3, 1)),
        "up": ([(0, 0), (1, 0), (2, -1)], (3, -2)),
        "out": ([(0, 0), (1, 0), (2, 0)], (3, -1)),
        "out_back": ([(0, 0), (-1, 0), (-2, 0)], (-3, -1)),
    }
    sleeve, (hx, hy) = paths[mode]
    for dx, dy in sleeve:
        cell.put(x + dx, y + dy, "D" if front else "X")
        cell.put(x + dx, y + dy + 1, "X")
    cell.put(x + hx, y + hy, "S" if front else "s")
    cell.put(x + hx, y + hy + 1, "s")


def body(style="rest", drop=0, head_drop=None, lean=0, legs=((-1, 0), (1, 0)),
         arms=("down", "down")):
    """Returns a function that draws the pose with a given flutter frame."""
    def draw(flutter):
        cell = Cell()
        head_y = 10 + (drop if head_drop is None else head_drop)
        head_x = 11 + lean
        torso_y = 19 + drop
        hip_y = min(torso_y + 3, SOLE_Y - 2)
        draw_tip(cell, head_x, head_y, style, flutter)
        draw_arm(cell, 14, torso_y, arms[0], front=False)
        draw_leg(cell, 14, hip_y, *legs[0], front=False)
        draw_leg(cell, 15, hip_y, *legs[1], front=True)
        # a short tunic under the mantle
        cell.rect(14, torso_y, 17, torso_y + 1, "M")
        cell.rect(14, torso_y, 14, torso_y + 1, "D")
        cell.rect(13, torso_y + 2, 17, torso_y + 2, "X")
        cell.put(16, torso_y + 2, "g")
        cell.art(12, torso_y - 1, ["C.", "Cc", "cc"])   # the satchel on its back
        cell.art(head_x, head_y, HEAD)
        draw_arm(cell, 16, torso_y, arms[1], front=True)
        return cell
    return draw


def crouch(flutter):
    """The box is 8 px tall while crouching: the hood sits on the boots."""
    cell = Cell()
    draw_tip(cell, 12, 15, "crouch", flutter)
    draw_leg(cell, 13, 24, -1, 0, front=False)
    draw_leg(cell, 16, 24, 0, 0, front=True)
    cell.rect(13, 23, 17, 24, "X")
    cell.art(11, 23, ["CC", "cc"])
    cell.art(12, 15, HEAD)
    draw_arm(cell, 17, 23, "forward", front=True)
    return cell


# Order = presentation::PlayerPose.
POSES = [
    # Idle: breathing.
    body(),
    body(),
    body(head_drop=1),
    body(head_drop=1),
    # Run: stride, push, passing; then the other leg.
    body("run", head_drop=1, lean=1, legs=((-4, 0), (3, 0)), arms=("forward", "backward")),
    body("run", drop=1, lean=1, legs=((-3, 2), (1, 0))),
    body("run", lean=1, legs=((1, 3), (0, 0)), arms=("backward", "forward")),
    body("run", head_drop=1, lean=1, legs=((3, 0), (-4, 0)), arms=("backward", "forward")),
    body("run", drop=1, lean=1, legs=((1, 0), (-3, 2))),
    body("run", lean=1, legs=((0, 0), (0, 3)), arms=("forward", "backward")),
    # Rise, apex, fall.
    body("rise", lean=1, legs=((-2, 2), (1, 3)), arms=("out_back", "up")),
    body("float", legs=((-2, 2), (2, 2)), arms=("out_back", "out")),
    body("fall", head_drop=1, legs=((-2, 0), (2, 1)), arms=("out_back", "up")),
    # Land.
    body("rest", drop=1, head_drop=2, legs=((-2, 0), (2, 0)), arms=("out_back", "out")),
    # Hurt.
    body("fall", lean=-2, legs=((-2, 1), (3, 1)), arms=("backward", "up")),
    # Slide.
    body("slide", drop=2, lean=-2, legs=((-1, 0), (5, 0)), arms=("backward", "forward")),
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
    background = (34, 30, 62, 255)
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
