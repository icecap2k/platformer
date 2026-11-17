#!/usr/bin/env python3
"""Draws the cave-rock tiles (standard library only).

    python3 game/tools/art/make_rock_tiles.py
    python3 game/tools/art/make_rock_tiles.py --preview preview.png   # a sample level, 6x enlarged

Writes:
  game/assets/sprites/rocks.png  the autotile sheet the game draws from
  game/assets/tiled/tiles.png    4 plain tiles, the palette shown in Tiled

Autotiles: every solid tile is drawn differently depending on which of its four
sides touch open air ("exposed"). The sheet has 16 columns, one per combination
(see presentation/autotile.h for the bit values) and 8 rows:

    rows 0-3  ground (tile id 1), four texture variants
    rows 4-7  rock   (any other id), four texture variants

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
from make_player_sprites import write_png  # noqa: E402

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


def build_sheet():
    sheet = [[(0, 0, 0, 0)] * (16 * SIZE) for _ in range(8 * SIZE)]
    for row, (material, variant) in enumerate((m, v) for m in (0, 1) for v in range(4)):
        for mask in range(16):
            tile = draw_tile(material, variant, mask)
            for y in range(SIZE):
                for x in range(SIZE):
                    sheet[row * SIZE + y][mask * SIZE + x] = tile[y][x]
    return sheet


def build_palette():
    """The four tiles Tiled shows: ground, rock, rock (variant) and a sunlit rock."""
    tiles = [draw_tile(0, 0, 0), draw_tile(1, 0, 0), draw_tile(1, 1, 0), draw_tile(1, 0, NORTH)]
    palette = [[(0, 0, 0, 0)] * (4 * SIZE) for _ in range(SIZE)]
    for index, tile in enumerate(tiles):
        for y in range(SIZE):
            for x in range(SIZE):
                palette[y][index * SIZE + x] = tile[y][x]
    return palette


# A little level to judge the shapes: bedrock floor, a thick rock block, a thin
# platform with two pointed ends, a one-tile boulder and a 2-tile ledge.
SAMPLE = [
    "..........................",
    "..........................",
    "..........................",
    "......RRRR.......RR.......",
    "......RRRRR...............",
    "..........................",
    "...RRRRRRR......R.....RRRR",
    "..........................",
    "GGGGGGGGGGGGGGGGGGGGGGGGGG",
    "GGGGGGGGGGGGGGGGGGGGGGGGGG",
]


def write_preview(path, scale=6):
    sheet = build_sheet()
    rows = len(SAMPLE)
    cols = len(SAMPLE[0])
    background = (70, 74, 120, 255)  # lighter than the cave so the outline can be judged
    canvas = [[background] * (cols * SIZE) for _ in range(rows * SIZE)]

    def solid(c, r):
        if c < 0 or r < 0 or c >= cols or r >= rows:
            return True  # outside the screen counts as solid (no outline on the edges)
        return SAMPLE[r][c] != "."

    for r in range(rows):
        for c in range(cols):
            if SAMPLE[r][c] == ".":
                continue
            mask = (NORTH if not solid(c, r - 1) else 0) | (SOUTH if not solid(c, r + 1) else 0) | \
                   (EAST if not solid(c + 1, r) else 0) | (WEST if not solid(c - 1, r) else 0)
            row = (0 if SAMPLE[r][c] == "G" else 4) + ((c * 7 + r * 13) % 4)
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
        print("wrote sprites/rocks.png (128x64) and tiled/tiles.png (32x8)")
