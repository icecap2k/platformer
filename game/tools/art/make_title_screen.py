#!/usr/bin/env python3
"""Makes the title picture, game/assets/background/title.png, from the author's drawing
game/art_sources/title.jpg (macOS only: it decodes and resizes with `sips`; the rest is standard
library).

    python3 game/tools/art/make_title_screen.py

The drawing is 16:9 and the screen 4:3, so a 4:3 slice is cut from it (keeping the heroine and the
mouth of the cave, to the right of the middle) and resized to 960x720: three times the logical
screen, which is the size of the window, so it is shown with all its detail.
"""

import struct
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_sprites import write_png  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "art_sources" / "title.jpg"
OUT = ROOT / "assets" / "background" / "title.png"
LEFT = 250          # where the 4:3 slice starts, in pixels of the source
WIDTH, HEIGHT = 960, 720


def read_bmp(path):
    data = path.read_bytes()
    offset = struct.unpack("<I", data[10:14])[0]
    width, height = struct.unpack("<ii", data[18:26])
    size = struct.unpack("<H", data[28:30])[0] // 8
    stride = (width * size + 3) // 4 * 4
    rows = []
    for y in range(abs(height)):
        row = abs(height) - 1 - y if height > 0 else y      # BMP rows usually go bottom up
        start = offset + row * stride
        rows.append([(data[start + x * size + 2], data[start + x * size + 1], data[start + x * size], 255)
                     for x in range(width)])
    return rows


def sips(*arguments):
    subprocess.run(["sips", *arguments], check=True, stdout=subprocess.DEVNULL)


def main():
    with tempfile.TemporaryDirectory() as folder:
        whole = Path(folder) / "whole.bmp"
        sips("-s", "format", "bmp", str(SOURCE), "--out", str(whole))
        pixels = read_bmp(whole)
        wide = len(pixels) * 4 // 3
        cut = Path(folder) / "cut.png"
        write_png(cut, [row[LEFT:LEFT + wide] for row in pixels])
        sips("-z", str(HEIGHT), str(WIDTH), str(cut), "--out", str(OUT))
    print(f"wrote {OUT} ({WIDTH}x{HEIGHT})")


if __name__ == "__main__":
    main()
