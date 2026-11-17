#!/usr/bin/env python3
"""Draws the cave scenery of game/assets/sprites/decor.png (standard library only).

    python3 game/tools/art/make_cave_decor.py
    python3 game/tools/art/make_cave_decor.py --preview preview.png   # 10x enlarged, does not write the sheet

Thirteen 8x8 frames, left to right, in the order the game expects
(see presentation/decoration.h and presentation/drip.h):

    0-5   grow on top of a rock: glowing mushrooms, crystals, pebbles
    6-7   hang from underneath a rock: a stalactite, glowing roots
    8-12  the stalactite's water drop: forming (small, large), falling, splash (two frames)

The drop frames are drawn relative to the stalactite frame (6): the drop forms
under its tip at column 3-4, row 5. While falling the renderer just moves that
frame down; the splash frames are drawn with their bottom row on the ground.

Each frame is ASCII art: '.' is transparent, 'O' the dark outline, and the other
letters are colours from that frame's own palette.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_player_sprites import write_png  # noqa: E402

OUT = Path(__file__).resolve().parents[2] / "assets" / "sprites" / "decor.png"
OUTLINE = (28, 20, 32, 255)


def rgb(r, g, b):
    return (r, g, b, 255)


FRAMES = [
    # 0: a glowing teal mushroom
    ({"a": rgb(52, 184, 176), "b": rgb(150, 248, 232), "d": rgb(30, 112, 124),
      "s": rgb(196, 204, 218), "t": rgb(130, 138, 164)},
     ["........",
      "........",
      "..OOOO..",
      ".ObbaaO.",
      ".OaaaadO",
      "..OsdO..",
      "..OstO..",
      ".OOstOO."]),
    # 1: two small pink mushrooms
    ({"a": rgb(214, 96, 170), "b": rgb(252, 176, 224), "d": rgb(138, 52, 112),
      "s": rgb(206, 196, 214), "t": rgb(140, 132, 160)},
     ["........",
      "........",
      ".OOO....",
      "ObbaO...",
      "OaadO.OO",
      ".OsO.ObO",
      ".OsOOOsO",
      ".OstOOtO"]),
    # 2: a cluster of cyan crystals
    ({"a": rgb(70, 200, 232), "b": rgb(196, 250, 255), "d": rgb(34, 120, 172)},
     ["...OO...",
      "..ObaO..",
      "..ObaO.O",
      ".OObadOa",
      ".ObbadOO",
      "OObaadOb",
      "ObbaaadO",
      "OOOOOOOO"]),
    # 3: a single violet crystal
    ({"a": rgb(158, 96, 214), "b": rgb(222, 178, 252), "d": rgb(98, 52, 150)},
     ["........",
      "...OO...",
      "..ObaO..",
      "..ObaO..",
      "..ObadO.",
      ".ObbaadO",
      ".ObaaadO",
      ".OOOOOOO"]),
    # 4: loose pebbles
    ({"a": rgb(120, 110, 134), "b": rgb(168, 158, 184), "d": rgb(80, 72, 98)},
     ["........",
      "........",
      "........",
      "........",
      "..OOO...",
      ".ObbaOOO",
      "ObaadObO",
      "OOOOOOOO"]),
    # 5: a tall pale mushroom with glowing spots
    ({"a": rgb(214, 198, 172), "b": rgb(255, 244, 214), "d": rgb(150, 130, 112),
      "g": rgb(120, 236, 196), "s": rgb(232, 224, 214)},
     ["..OOO...",
      ".ObgaO..",
      "ObaagdO.",
      "OaaddaO.",
      ".OOsdO..",
      "...OsdO.",
      "...OsdO.",
      "..OOsOO."]),
    # 6 (hanging): a small stalactite (its drop is animated by frames 8-12)
    ({"a": rgb(124, 114, 140), "b": rgb(168, 158, 184), "d": rgb(84, 76, 100)},
     [".OaaaaO.",
      ".ObbadO.",
      "..ObdO..",
      "..OadO..",
      "...OO...",
      "........",
      "........",
      "........"]),
    # 7 (hanging): glowing roots
    ({"a": rgb(44, 124, 96), "b": rgb(130, 236, 176), "d": rgb(26, 80, 70)},
     [".OOO.OO.",
      ".OaOOadO",
      ".OaO.OaO",
      "..OdO.Od",
      "..Oa..OO",
      "..Ob....",
      "...O....",
      "........"]),
    # 8: the drop forms at the tip (small)
    ({"w": rgb(140, 212, 255), "W": rgb(222, 246, 255)},
     ["........",
      "........",
      "........",
      "........",
      "........",
      "...ww...",
      "........",
      "........"]),
    # 9: the drop swells (large)
    ({"w": rgb(140, 212, 255), "W": rgb(222, 246, 255)},
     ["........",
      "........",
      "........",
      "........",
      "........",
      "...Ww...",
      "...ww...",
      "........"]),
    # 10: falling, a little stretched
    ({"w": rgb(140, 212, 255), "W": rgb(222, 246, 255)},
     ["........",
      "........",
      "........",
      "........",
      "........",
      "...W....",
      "...w....",
      "........"]),
    # 11: splash, droplets jump up
    ({"w": rgb(140, 212, 255), "W": rgb(222, 246, 255)},
     ["........",
      "........",
      "........",
      "........",
      "........",
      "..w..w..",
      "...WW...",
      "..wwww.."]),
    # 12: splash, spreading out
    ({"w": rgb(140, 212, 255), "W": rgb(222, 246, 255)},
     ["........",
      "........",
      "........",
      "........",
      ".w....w.",
      "........",
      "..w..w..",
      ".wwwwww."]),
]


def render(frames):
    sheet = [[(0, 0, 0, 0)] * (8 * len(frames)) for _ in range(8)]
    for index, (palette, rows) in enumerate(frames):
        assert len(rows) == 8, f"frame {index} must have 8 rows"
        for y, row in enumerate(rows):
            assert len(row) == 8, f"frame {index}, row {y} must have 8 characters: {row!r}"
            for x, char in enumerate(row):
                if char == ".":
                    continue
                color = OUTLINE if char == "O" else palette[char]
                sheet[y][index * 8 + x] = color
    return sheet


def write_preview(path, sheet, scale=10):
    background = (70, 74, 120, 255)
    rows = []
    for row in sheet:
        wide = []
        for px in row:
            wide.extend([px if px[3] else background] * scale)
        rows.extend([wide] * scale)
    write_png(path, rows)


if __name__ == "__main__":
    sheet = render(FRAMES)
    if "--preview" in sys.argv:
        write_preview(Path(sys.argv[sys.argv.index("--preview") + 1]), sheet)
    else:
        write_png(OUT, sheet)
        print(f"wrote {OUT} ({len(FRAMES)} frames)")
