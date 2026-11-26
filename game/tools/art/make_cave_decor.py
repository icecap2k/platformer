#!/usr/bin/env python3
"""Draws the cave scenery of game/assets/sprites/decor.png (standard library only).

    python3 game/tools/art/make_cave_decor.py
    python3 game/tools/art/make_cave_decor.py --preview preview.png   # 6x enlarged, does not write the sheet

Sheet layout (see presentation/decoration.h and presentation/drip.h):

    row 0 (y = 0):  one 32x32 cell per prop, in the order of `presentation::PropType`; each prop
                    sits in the top left corner of its cell and measures a whole number of tiles.
                    0-7 grow on the floor, 8-11 hang from the ceiling, 12-15 stick out of a wall
                    (drawn with the rock on their LEFT; the renderer mirrors them for the other side).
    row 1 (y = 32): the 8x8 frames of the water drop of the stalactite cluster: forming (small,
                    large), falling, splash (two frames).

Every prop is built from shapes (caps, crystals, stalks...). Each shape gets its own dark outline
before it is pasted on the prop, so overlapping pieces stay readable; glowing ones add a soft halo.

Scenery must read as BACKGROUND, never as something that hurts or can be picked up: when the sheet
is written every colour (outline included) is faded towards the colour of the cave, and the halos
are dimmed. The pointed rocks, the closest in shape to the spikes, are faded the most.
"""

import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_sprites import write_png  # noqa: E402

OUT = Path(__file__).resolve().parents[2] / "assets" / "sprites" / "decor.png"
OUTLINE = (28, 20, 32, 255)
CELL = 32
DROP = 8

# Colour ramps: (light, mid, dark)
TEAL = ((156, 246, 226, 255), (56, 176, 172, 255), (30, 100, 118, 255))
PINK = ((250, 178, 226, 255), (204, 92, 168, 255), (126, 50, 112, 255))
CYAN = ((204, 250, 255, 255), (80, 192, 230, 255), (36, 108, 162, 255))
VIOLET = ((226, 188, 252, 255), (156, 100, 212, 255), (92, 52, 146, 255))
ROCK = ((164, 154, 184, 255), (116, 106, 136, 255), (76, 68, 96, 255))
STEM = ((226, 228, 236, 255), (176, 178, 200, 255), (118, 118, 148, 255))
LEAF = ((140, 230, 170, 255), (54, 142, 108, 255), (28, 88, 80, 255))
BUD = ((236, 255, 220, 255), (164, 244, 190, 255), (80, 180, 140, 255))


class Canvas:
    def __init__(self, width, height):
        self.w, self.h = width, height
        self.px = {}

    def put(self, x, y, colour):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[(x, y)] = colour

    def paste(self, shape, outline=True):
        """Puts a shape (a dict of pixels) on the canvas, with its own outline around it."""
        if outline:
            for (x, y) in shape:
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    if (x + dx, y + dy) not in shape:
                        self.put(x + dx, y + dy, OUTLINE)
        for (x, y), colour in shape.items():
            self.put(x, y, colour)

    def halo(self, cx, cy, radius, rgb, peak):
        """A soft glow, painted only where nothing else is."""
        for y in range(self.h):
            for x in range(self.w):
                distance = math.hypot(x + 0.5 - cx, y + 0.5 - cy)
                if distance < radius and (x, y) not in self.px:
                    alpha = round(peak * (1.0 - distance / radius))
                    if alpha > 6:
                        self.px[(x, y)] = rgb + (alpha,)


# --------------------------------------------------------------------------- shapes

def polygon(points):
    """The pixels whose centre lies inside the polygon."""
    xs, ys = [p[0] for p in points], [p[1] for p in points]
    inside = []
    for y in range(int(min(ys)), int(max(ys)) + 1):
        for x in range(int(min(xs)), int(max(xs)) + 1):
            px, py, hit = x + 0.5, y + 0.5, False
            for (ax, ay), (bx, by) in zip(points, points[1:] + points[:1]):
                if (ay > py) != (by > py) and px < ax + (py - ay) * (bx - ax) / (by - ay):
                    hit = not hit
            if hit:
                inside.append((x, y))
    return inside


def crystal(canvas, base, tip, half, ramp):
    """A faceted point from `base` to `tip`: one lit face, one shaded face and a bright ridge."""
    (bx, by), (tx, ty) = base, tip
    dx, dy = tx - bx, ty - by
    length = math.hypot(dx, dy)
    nx, ny = -dy / length, dx / length  # to the left of the direction of growth
    if nx > 0 or (nx == 0 and ny > 0):
        nx, ny = -nx, -ny
    shoulder = (bx + dx * 0.68, by + dy * 0.68)
    points = [(bx + nx * half, by + ny * half), (shoulder[0] + nx * half, shoulder[1] + ny * half),
              (tx, ty), (shoulder[0] - nx * half, shoulder[1] - ny * half), (bx - nx * half, by - ny * half)]
    shape = {}
    for (x, y) in polygon(points):
        side = (x + 0.5 - bx) * nx + (y + 0.5 - by) * ny
        shape[(x, y)] = ramp[0] if side > half * 0.45 else (ramp[1] if side > -half * 0.2 else ramp[2])
    canvas.paste(shape)


def mushroom(canvas, cx, base, stem_height, rx, ry, ramp, spots=()):
    """A stalk from `base` and a domed cap on top of it."""
    cap_y = base - stem_height
    stem = {}
    for y in range(cap_y, base + 1):
        wide = max(1, rx // 3) + (1 if y >= base - 1 else 0)
        for x in range(cx - wide, cx + wide):
            stem[(x, y)] = STEM[0] if x < cx - wide + 1 else (STEM[1] if x < cx + wide - 1 else STEM[2])
    canvas.paste(stem)
    cap = {}
    for y in range(cap_y - ry, cap_y + 2):
        for x in range(cx - rx, cx + rx):
            u, v = (x + 0.5 - cx) / rx, (y + 0.5 - (cap_y + 1)) / (ry + 1)
            if u * u + v * v <= 1.0:
                light = -u * 0.7 - v * 0.9
                cap[(x, y)] = ramp[0] if light > 0.75 else ramp[1]
                if y >= cap_y or u > 0.72:
                    cap[(x, y)] = ramp[2]
    for sx, sy in spots:
        if (cx + sx, cap_y + sy) in cap:
            cap[(cx + sx, cap_y + sy)] = (255, 250, 230, 255)
    canvas.paste(cap)


def spire(canvas, left, right, base, tip, ramp):
    """A rock point (a stalagmite if the tip is above the base, a stalactite if below)."""
    tx, ty = tip
    shape = {}
    low, high = sorted((base, ty))
    for y in range(low, high + 1):
        t = abs(y - base) / max(1, abs(ty - base))  # 0 at the base, 1 at the tip
        a = left + (tx - left) * t ** 0.85
        b = right + (tx + 1 - right) * t ** 0.85
        for x in range(int(round(a)), max(int(round(a)) + 1, int(round(b)))):
            u = (x + 0.5 - a) / max(1.0, b - a)
            colour = ramp[0] if u < 0.34 else (ramp[1] if u < 0.7 else ramp[2])
            if (y + x // 3) % 6 == 0 and 0.2 < u < 0.85:
                colour = ramp[2]  # a crack between two layers of stone
            shape[(x, y)] = colour
    canvas.paste(shape)


def strand(canvas, points, ramp, leaves=(), bud=True):
    """A vine or a stem: a 1 px line through `points`, with small leaves and a glowing end."""
    shape = {}
    for (ax, ay), (bx, by) in zip(points, points[1:]):
        steps = max(abs(bx - ax), abs(by - ay), 1)
        for i in range(steps + 1):
            shape[(round(ax + (bx - ax) * i / steps), round(ay + (by - ay) * i / steps))] = ramp[1]
    for x, y, side in leaves:
        shape[(x + side, y)] = ramp[1]
        shape[(x + 2 * side, y - 1)] = ramp[0]
        shape[(x + side, y - 1)] = ramp[0]
    canvas.paste(shape)
    if bud:
        x, y = points[-1]
        canvas.paste({(x, y): BUD[0], (x + 1, y): BUD[1], (x, y + 1): BUD[1], (x + 1, y + 1): BUD[2]})


# ---------------------------------------------------------------------------- props
# Order = presentation::PropType.

def giant_mushroom():
    c = Canvas(24, 24)
    mushroom(c, 10, 23, 13, 9, 7, TEAL, spots=((-5, -3), (-1, -5), (3, -2), (-3, -1)))
    mushroom(c, 19, 23, 5, 4, 3, TEAL, spots=((-1, -1),))
    c.halo(10, 8, 13, (90, 230, 210), 90)
    return c


def pink_mushrooms():
    c = Canvas(16, 16)
    mushroom(c, 5, 15, 8, 5, 4, PINK, spots=((-2, -2), (1, -1)))
    mushroom(c, 12, 15, 4, 3, 3, PINK, spots=((0, -1),))
    c.halo(7, 7, 9, (240, 130, 210), 80)
    return c


def cyan_crystals():
    c = Canvas(24, 24)
    crystal(c, (6, 24), (2, 11), 2.6, CYAN)
    crystal(c, (17, 24), (22, 9), 2.8, CYAN)
    crystal(c, (11, 24), (11, 2), 3.8, CYAN)
    crystal(c, (15, 24), (18, 17), 2.0, CYAN)
    crystal(c, (7, 24), (6, 18), 1.8, CYAN)
    c.halo(12, 13, 14, (110, 210, 255), 80)
    return c


def violet_crystals():
    c = Canvas(16, 16)
    crystal(c, (4, 16), (2, 7), 2.0, VIOLET)
    crystal(c, (9, 16), (11, 2), 2.8, VIOLET)
    crystal(c, (12, 16), (14, 10), 1.6, VIOLET)
    c.halo(8, 9, 9, (180, 130, 250), 70)
    return c


def stalagmites():
    c = Canvas(16, 24)
    spire(c, 5, 14, 23, (9, 2), ROCK)
    spire(c, 1, 7, 23, (3, 12), ROCK)
    spire(c, 11, 16, 23, (13, 16), ROCK)
    return c


def glow_plant():
    c = Canvas(16, 16)
    strand(c, [(7, 15), (5, 10), (2, 8)], LEAF, leaves=((5, 11, -1),))
    strand(c, [(8, 15), (10, 8), (12, 6)], LEAF, leaves=((9, 11, 1),))
    strand(c, [(7, 15), (7, 7), (6, 3)], LEAF, leaves=((7, 9, -1), (7, 12, 1)))
    strand(c, [(9, 15), (12, 12), (13, 11)], LEAF)
    c.halo(7, 5, 8, (170, 255, 200), 90)
    return c


def pebbles():
    c = Canvas(16, 8)
    for cx, rx, ry in ((4, 3, 3), (11, 4, 4), (7, 2, 2)):
        shape = {}
        for y in range(7 - ry, 8):
            for x in range(cx - rx, cx + rx):
                u, v = (x + 0.5 - cx) / rx, (y + 0.5 - 8) / (ry + 0.5)
                if u * u + v * v <= 1.0:
                    shape[(x, y)] = ROCK[0] if -u - v > 0.9 else (ROCK[2] if u > 0.4 else ROCK[1])
        c.paste(shape)
    return c


def small_mushroom():
    c = Canvas(8, 8)
    mushroom(c, 4, 7, 3, 3, 2, TEAL)
    return c


def stalactite_cluster():
    """Its longest point ends at (4, 22): the water drop forms right under it."""
    c = Canvas(16, 24)
    spire(c, 8, 16, 0, (11, 13), ROCK)
    spire(c, 0, 10, 0, (4, 22), ROCK)
    spire(c, 7, 12, 0, (9, 7), ROCK)
    return c


def vines():
    c = Canvas(16, 32)
    strand(c, [(3, -1), (4, 6), (2, 12), (3, 19), (4, 24)], LEAF, leaves=((4, 5, 1), (2, 11, -1), (3, 17, 1)))
    strand(c, [(9, -1), (8, 8), (10, 15), (9, 22), (10, 29)], LEAF,
           leaves=((8, 6, -1), (10, 14, 1), (9, 20, -1), (9, 25, 1)))
    strand(c, [(13, -1), (13, 5), (12, 10)], LEAF, leaves=((13, 4, 1),))
    c.halo(10, 29, 7, (170, 255, 200), 90)
    c.halo(4, 24, 6, (170, 255, 200), 70)
    return c


def roots():
    c = Canvas(24, 16)
    brown = ((190, 150, 128, 255), (132, 98, 92, 255), (88, 64, 70, 255))
    strand(c, [(4, -1), (5, 4), (2, 8), (3, 12)], brown, bud=False)
    strand(c, [(5, 3), (9, 7), (8, 11)], brown, bud=False)
    strand(c, [(14, -1), (13, 5), (15, 9), (13, 14)], brown, bud=False)
    strand(c, [(14, 6), (18, 8), (19, 11)], brown, bud=False)
    strand(c, [(20, -1), (21, 4), (20, 6)], brown, bud=False)
    return c


def hanging_crystals():
    c = Canvas(16, 16)
    crystal(c, (4, -1), (2, 8), 2.0, VIOLET)
    crystal(c, (12, -1), (14, 9), 2.0, CYAN)
    crystal(c, (8, -1), (8, 14), 3.0, CYAN)
    c.halo(8, 8, 9, (130, 200, 255), 70)
    return c


def shelf(canvas, y, reach, thick, ramp):
    """A half cap growing out of the rock on the left."""
    shape = {}
    for yy in range(y - thick, y + 2):
        for x in range(0, reach):
            u, v = (x + 0.5) / reach, (yy + 0.5 - (y + 1)) / (thick + 1)
            if u * u + v * v <= 1.0:
                shape[(x, yy)] = ramp[2] if yy >= y else (ramp[0] if v < -0.55 else ramp[1])
    for x in range(2, reach - 2, 4):
        if (x, y - 2) in shape:
            shape[(x, y - 2)] = (255, 250, 230, 255)
    canvas.paste(shape)


def shelf_mushrooms():
    c = Canvas(16, 24)
    shelf(c, 7, 10, 4, PINK)
    shelf(c, 15, 14, 5, PINK)
    shelf(c, 21, 7, 3, PINK)
    c.halo(5, 13, 12, (240, 130, 210), 70)
    return c


def wall_crystals():
    c = Canvas(16, 16)
    crystal(c, (-1, 5), (9, 1), 2.2, CYAN)
    crystal(c, (-1, 12), (9, 14), 2.0, CYAN)
    crystal(c, (-1, 9), (14, 7), 3.0, CYAN)
    c.halo(5, 8, 10, (110, 210, 255), 70)
    return c


def wall_vine():
    c = Canvas(8, 32)
    strand(c, [(0, 1), (2, 6), (0, 12), (2, 18), (1, 24), (2, 28)], LEAF,
           leaves=((2, 6, 1), (1, 10, 1), (2, 17, 1), (1, 23, 1)))
    c.halo(3, 28, 5, (170, 255, 200), 80)
    return c


def wall_stalks():
    c = Canvas(8, 16)
    for y, reach in ((4, 5), (12, 6)):
        strand(c, [(0, y + 2), (2, y + 1), (reach - 2, y - 1)], STEM, bud=False)
        c.paste({(reach - 2 + dx, y - 2 + dy): (TEAL[0] if dy == 0 else TEAL[1])
                 for dx in range(2) for dy in range(2)})
    c.halo(4, 6, 8, (90, 230, 210), 70)
    return c


PROPS = [giant_mushroom, pink_mushrooms, cyan_crystals, violet_crystals, stalagmites, glow_plant, pebbles,
         small_mushroom, stalactite_cluster, vines, roots, hanging_crystals, shelf_mushrooms, wall_crystals,
         wall_vine, wall_stalks]

# The water drop, relative to the stalactite's tip: it forms at column 3-4, row 5 of its frame.
WATER = {"w": (140, 212, 255, 255), "W": (222, 246, 255, 255)}
DROPS = [
    ["........", "........", "........", "........", "........", "...ww...", "........", "........"],
    ["........", "........", "........", "........", "........", "...Ww...", "...ww...", "........"],
    ["........", "........", "........", "........", "........", "...W....", "...w....", "........"],
    ["........", "........", "........", "........", "........", "..w..w..", "...WW...", "..wwww.."],
    ["........", "........", "........", "........", ".w....w.", "........", "..w..w..", ".wwwwww."],
]


HAZE = (36, 31, 66)     # the colour of the cave behind the scenery
FADE = 0.42             # how far every prop is pushed towards it (0 = full colour)
ROCK_FADE = 0.58        # stalagmites and stalactites: the shapes that could pass for spikes
HALO_STRENGTH = 0.5


def faded(colour, amount):
    r, g, b, a = colour
    if a < 255:  # a halo pixel: just dimmer
        return (r, g, b, round(a * HALO_STRENGTH))
    return tuple(round(c + (h - c) * amount) for c, h in zip((r, g, b), HAZE)) + (a,)


def render():
    sheet = [[(0, 0, 0, 0)] * (CELL * len(PROPS)) for _ in range(CELL + DROP)]
    for index, build in enumerate(PROPS):
        amount = ROCK_FADE if build in (stalagmites, stalactite_cluster) else FADE
        for (x, y), colour in build().px.items():
            sheet[y][index * CELL + x] = faded(colour, amount)
    for index, rows in enumerate(DROPS):
        for y, row in enumerate(rows):
            for x, char in enumerate(row):
                if char != ".":
                    sheet[CELL + y][index * DROP + x] = WATER[char]
    return sheet


def write_preview(path, sheet, scale=6):
    background = (38, 34, 66, 255)
    per_row = 8
    out = []
    for first in (0, per_row):
        for row in sheet[:CELL]:
            line = []
            for px in row[first * CELL:(first + per_row) * CELL]:
                alpha = px[3] / 255
                mixed = tuple(round(px[i] * alpha + background[i] * (1 - alpha)) for i in range(3)) + (255,)
                line.extend([mixed] * scale)
            out.extend([line] * scale)
    write_png(path, out)


if __name__ == "__main__":
    sheet = render()
    if "--preview" in sys.argv:
        write_preview(Path(sys.argv[sys.argv.index("--preview") + 1]), sheet)
    else:
        write_png(OUT, sheet)
        print(f"wrote {OUT} ({len(PROPS)} props)")
