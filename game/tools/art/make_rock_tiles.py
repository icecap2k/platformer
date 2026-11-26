#!/usr/bin/env python3
"""Draws the cave-rock tiles (standard library only).

    python3 game/tools/art/make_rock_tiles.py
    python3 game/tools/art/make_rock_tiles.py --preview preview.png   # a sample level, 6x enlarged

Writes:
  game/assets/sprites/rocks.png  the autotile sheet the game draws from
  game/assets/tiled/tiles.png    6 plain tiles, the palette shown in Tiled (ids 1-4 blocks, 5-6 ramps)

Autotiles: every solid tile is drawn differently depending on which of its four
sides touch open air ("exposed"). The sheet has 16 columns, one per combination
(see presentation/autotile.h for the bit values) and 10 rows:

    rows 0-3  ground (tile id 1), four texture variants
    rows 4-7  rock   (any other id), four texture variants
    row 8     ramp rising to the right (tile id 5), 4 columns
    row 9     ramp rising to the left  (tile id 6), 4 columns

A ramp is a 45 degree triangle (see ADR-0010). Its column is a 2-bit mask: bit 0 =
the south face is exposed (nothing under it), bit 1 = the face on its high side is
exposed (east for id 5, west for id 6). The hypotenuse always has the outline, with
a lit line under it, as it is the surface the player walks on.

Shapes follow the cave's rocks: a platform ends in a pointed tip that tapers
underneath (thinner than the tile), corners on top are rounded, and the
underside is jagged like small stalactites. The dark outline is part of the art
(it follows the carved shapes), so the renderer draws no outline of its own.
The collision is NOT carved: it stays a full tile, and the flat top of every
tile is kept so the player always seems to stand on the rock.
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_sprites import write_png  # noqa: E402

SIZE = 8
NORTH, SOUTH, EAST, WEST = 1, 2, 4, 8  # keep in sync with presentation/autotile.h

ROOT = Path(__file__).resolve().parents[2] / "assets"
OUTLINE = (28, 20, 32, 255)

MATERIALS = [
    # (base, light, dark, top highlight): bedrock, a cool dark slate
    ((64, 60, 96, 255), (88, 84, 126, 255), (46, 42, 74, 255), (112, 108, 156, 255)),
    # rock: lighter, a little warmer
    ((100, 90, 110, 255), (130, 120, 142, 255), (70, 62, 86, 255), (164, 154, 176, 255)),
]


def texture(material, variant):
    """An 8x8 block of stone, without any shaping: speckles, facets and a crack."""
    base, light, dark, _ = MATERIALS[material]
    rng = random.Random(1000 + material * 10 + variant)  # 4 variants per material
    px = [[base] * SIZE for _ in range(SIZE)]
    for _ in range(3):  # lighter facets on the upper left of a few stones
        cx, cy = rng.randint(1, 6), rng.randint(1, 6)
        for dx, dy in ((0, 0), (1, 0), (0, 1)):
            px[(cy + dy) % SIZE][(cx + dx) % SIZE] = light
    for _ in range(4):  # shaded pixels
        px[rng.randint(0, 7)][rng.randint(0, 7)] = dark
    x, y = rng.randint(1, 4), rng.randint(2, 4)
    for _ in range(3):  # a short crack running down and to the right
        px[y % SIZE][x % SIZE] = dark
        x += rng.choice((0, 1))
        y += 1
    return px


def shape(mask, variant):
    """Which pixels of the tile are rock, for this exposure mask."""
    keep = [[True] * SIZE for _ in range(SIZE)]
    n, s, e, w = (bool(mask & bit) for bit in (NORTH, SOUTH, EAST, WEST))

    def cut(x, y):
        keep[y][x] = False

    if n and w:                      # rounded top corners
        cut(0, 0)
    if n and e:
        cut(7, 0)
    if s and w:                      # the underside tapers into a tip: thinner at the end
        for x in range(SIZE):
            for y in range(SIZE):
                if y > 3 + x:
                    cut(x, y)
    if s and e:
        for x in range(SIZE):
            for y in range(SIZE):
                if y > 3 + (7 - x):
                    cut(x, y)
    if s and not w and not e:        # small stalactites hanging from the underside
        nubs = ((2, 5), (1, 4, 6), (3, 6), (1, 5))[variant]
        for x in range(SIZE):
            if x not in nubs:
                cut(x, 7)
        if variant % 2 == 0:
            cut(0, 7)
            cut(0, 6)
        else:
            cut(7, 6)
    return keep


def draw_tile(material, variant, mask):
    base, light, dark, top = MATERIALS[material]
    tex = texture(material, variant)
    keep = shape(mask, variant)
    n, s, e, w = (bool(mask & bit) for bit in (NORTH, SOUTH, EAST, WEST))

    def empty(x, y):
        """Is the pixel (x, y), possibly outside this tile, open air?"""
        if y < 0:
            return n
        if y >= SIZE:
            return s
        if x < 0:
            return w
        if x >= SIZE:
            return e
        return not keep[y][x]

    out = [[(0, 0, 0, 0)] * SIZE for _ in range(SIZE)]
    for y in range(SIZE):
        for x in range(SIZE):
            if not keep[y][x]:
                continue
            color = tex[y][x]
            if n and y == 1:
                color = top                                    # sunlit top face
            if s and y == 6 and keep[6][x]:
                color = dark                                   # shadowed underside
            if any(empty(x + dx, y + dy) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                color = OUTLINE
            out[y][x] = color
    return out


def draw_slope(rises_right, mask):
    """An 8x8 ramp tile. Pixel (x, y) is rock when it is on or under the diagonal."""
    base, light, dark, top = MATERIALS[1]
    tex = texture(1, 0)
    south_exposed = bool(mask & 1)
    high_side_exposed = bool(mask & 2)
    out = [[(0, 0, 0, 0)] * SIZE for _ in range(SIZE)]
    for y in range(SIZE):
        for x in range(SIZE):
            column = x if rises_right else SIZE - 1 - x     # distance from the low end
            if y < SIZE - 1 - column:
                continue                                    # above the surface
            if y == SIZE - 1 - column:
                color = OUTLINE                             # the surface itself
            elif y == SIZE - column:
                color = top                                 # lit line under it
            else:
                color = tex[y][x]
            if south_exposed and y == SIZE - 1:
                color = OUTLINE
            high_edge = SIZE - 1 if rises_right else 0
            if high_side_exposed and x == high_edge:
                color = OUTLINE
            out[y][x] = color
    return out


def build_sheet():
    sheet = [[(0, 0, 0, 0)] * (16 * SIZE) for _ in range(10 * SIZE)]
    for row, (material, variant) in enumerate((m, v) for m in (0, 1) for v in range(4)):
        for mask in range(16):
            tile = draw_tile(material, variant, mask)
            for y in range(SIZE):
                for x in range(SIZE):
                    sheet[row * SIZE + y][mask * SIZE + x] = tile[y][x]
    for row, rises_right in ((8, True), (9, False)):
        for mask in range(4):
            tile = draw_slope(rises_right, mask)
            for y in range(SIZE):
                for x in range(SIZE):
                    sheet[row * SIZE + y][mask * SIZE + x] = tile[y][x]
    return sheet


def build_palette():
    """The six tiles Tiled shows: ids 1-4 (ground, rock, rock variant, sunlit rock), then
    id 5 (ramp rising to the right) and id 6 (ramp rising to the left)."""
    tiles = [draw_tile(0, 0, 0), draw_tile(1, 0, 0), draw_tile(1, 1, 0), draw_tile(1, 0, NORTH),
             draw_slope(True, 0), draw_slope(False, 0)]
    palette = [[(0, 0, 0, 0)] * (6 * SIZE) for _ in range(SIZE)]
    for index, tile in enumerate(tiles):
        for y in range(SIZE):
            for x in range(SIZE):
                palette[y][index * SIZE + x] = tile[y][x]
    return palette


# A little level to judge the shapes: bedrock floor ('G'), rock ('R'), ramps rising to
# the right ('/') and to the left ('\\'), and a floating platform.
def make_sample():
    """Floor, a platform, a 4-tile ramp rising to the right (with its filler blocks and a
    plateau) and the mirror image of it rising to the left."""
    width, height = 26, 10
    grid = [["."] * width for _ in range(height)]
    for row in (8, 9):
        grid[row] = ["G"] * width
    for c in range(11, 17):
        grid[3][c] = "R"
    for i in range(4):                       # the ramp: 4 tiles, rising 32 px
        ramp_row = 7 - i
        grid[ramp_row][3 + i] = "/"
        grid[ramp_row][width - 1 - (3 + i)] = "\\"
        for row in range(ramp_row + 1, 8):   # filler under the ramp
            grid[row][3 + i] = "R"
            grid[row][width - 1 - (3 + i)] = "R"
    for c in range(7, 11):                   # the plateaus, with filler under them
        for row in range(4, 8):
            grid[row][c] = "R"
            grid[row][width - 1 - c] = "R"
    return ["".join(line) for line in grid]


SAMPLE = make_sample()


def write_preview(path, scale=6):
    sheet = build_sheet()
    rows = len(SAMPLE)
    cols = len(SAMPLE[0])
    background = (70, 74, 120, 255)  # lighter than the cave so the outline can be judged
    canvas = [[background] * (cols * SIZE) for _ in range(rows * SIZE)]

    def covers(c, r, face):
        """Does the cell fill this face? Outside the screen counts as filled."""
        if c < 0 or r < 0 or c >= cols or r >= rows:
            return True
        ch = SAMPLE[r][c]
        if ch in "GR":
            return True
        if ch == "/":
            return face in (SOUTH, EAST)
        if ch == "\\":
            return face in (SOUTH, WEST)
        return False

    for r in range(rows):
        for c in range(cols):
            ch = SAMPLE[r][c]
            if ch == ".":
                continue
            south_exposed = not covers(c, r + 1, NORTH)
            if ch in "/\\":
                side_exposed = not (covers(c + 1, r, WEST) if ch == "/" else covers(c - 1, r, EAST))
                row = 8 if ch == "/" else 9
                mask = (1 if south_exposed else 0) | (2 if side_exposed else 0)
            else:
                mask = (NORTH if not covers(c, r - 1, SOUTH) else 0) | (SOUTH if south_exposed else 0) | \
                       (EAST if not covers(c + 1, r, WEST) else 0) | (WEST if not covers(c - 1, r, EAST) else 0)
                row = (0 if ch == "G" else 4) + ((c * 7 + r * 13) % 4)
            for y in range(SIZE):
                for x in range(SIZE):
                    px = sheet[row * SIZE + y][mask * SIZE + x]
                    if px[3]:
                        canvas[r * SIZE + y][c * SIZE + x] = px
    big = []
    for line in canvas:
        wide = [px for px in line for _ in range(scale)]
        big.extend([wide] * scale)
    write_png(path, big)


if __name__ == "__main__":
    if "--preview" in sys.argv:
        write_preview(Path(sys.argv[sys.argv.index("--preview") + 1]))
    else:
        write_png(ROOT / "sprites" / "rocks.png", build_sheet())
        write_png(ROOT / "tiled" / "tiles.png", build_palette())
        print("wrote sprites/rocks.png (128x80) and tiled/tiles.png (48x8)")
