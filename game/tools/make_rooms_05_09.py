#!/usr/bin/env python3
"""Writes the text sources of rooms 05-09 (game/levels/room_0N.txt).

    python3 game/tools/make_rooms_05_09.py
    for n in 5 6 7 8 9; do ./build/level_from_text game/levels/room_0$n.txt game/assets/tiled/room_0$n.tmj; done

Each room is built from a few calls on a 40 x 30 grid (one character per 8 px tile) and then
checked by `level_check`. Edit this script, not the generated files.
"""

from pathlib import Path

W, H = 40, 30
OUT = Path(__file__).resolve().parents[1] / "levels"


class Room:
    def __init__(self):
        self.g = [["."] * W for _ in range(H)]

    def put(self, col, row, ch):
        self.g[row][col] = ch

    def fill(self, c0, c1, r0, r1, ch):
        for r in range(r0, r1 + 1):
            for c in range(c0, c1 + 1):
                self.g[r][c] = ch

    def floor(self, gaps=()):
        self.fill(0, W - 1, 27, 29, "#")
        for a, b in gaps:
            self.fill(a, b, 27, 29, ".")

    def platform(self, c0, c1, row, ch="B"):
        self.fill(c0, c1, row, row, ch)

    def text(self, header):
        return header + "---\n" + "\n".join("".join(r) for r in self.g) + "\n"


def room_05():
    """The hall: a spiked pit, a patrolled stretch and a checkpoint."""
    r = Room()
    r.floor(gaps=[(13, 17)])
    r.fill(13, 17, 29, 29, "^")           # spikes at the bottom of the pit
    r.platform(12, 17, 22)               # a platform over the pit...
    r.put(14, 21, "C")                   # ...with a coin on it
    r.put(26, 26, "E")                   # a walker
    r.fill(30, 30, 25, 26, "B")          # a low wall that stops it
    r.put(28, 26, "C")
    r.put(35, 26, "K")
    r.put(22, 25, "F")
    return r.text("west: room_04\neast: room_06\n")


def room_06():
    """The climb: platforms zig-zag up to the north exit."""
    r = Room()
    r.floor()
    r.put(3, 26, "K")
    r.fill(22, 27, 26, 26, "^")          # spikes under the first jumps
    r.platform(6, 11, 23); r.put(8, 22, "C")
    r.platform(16, 21, 19); r.put(18, 18, "E")
    r.platform(6, 11, 15); r.put(8, 14, "C")
    r.platform(16, 21, 11); r.put(18, 9, "F")
    r.platform(26, 31, 7); r.put(28, 6, "C")
    r.platform(33, 38, 3)                # the top step: jump north from it
    return r.text("west: room_05\nnorth: room_07\n")


def room_07():
    """The cavern floor: you come up through a gap at the east and head west past spikes, traps and a walker."""
    r = Room()
    r.floor(gaps=[(32, 37)])            # you come up through this opening (like room_03)
    r.put(13, 16, "C")
    r.platform(1, 6, 22)                 # a step up to the hanging platform
    r.fill(24, 26, 26, 26, "^")
    r.platform(8, 18, 17, "B")           # hanging platform with a stalactite trap under it
    r.put(12, 18, "V")
    r.put(15, 18, "V")
    r.put(10, 26, "E")
    r.fill(20, 20, 25, 26, "B")
    r.put(28, 25, "F")
    r.put(4, 26, "C")
    return r.text("south: room_06\nwest: room_08\n")


def room_08():
    """The hump: climb the ramp from the east, cross the plateau, drop and cross the spikes."""
    r = Room()
    r.floor()
    # plateau at rows 22-26 on the left of the ramp, ramp rising towards the left (tile 6)
    for k in range(5):
        row = 22 + k
        r.fill(12, 18 + k, row, row, "4" if k == 0 else "B")
        r.put(19 + k, row, "\\")
    r.fill(12, 18, 22, 22, "4")
    r.put(14, 21, "K")
    r.put(16, 21, "C")
    r.fill(7, 9, 26, 26, "^")
    r.put(4, 26, "E")
    r.put(1, 26, "C")
    r.put(33, 25, "F")
    return r.text("east: room_07\nwest: room_09\n")


def room_09():
    """The last room: a pit, stalactite traps and the exit."""
    r = Room()
    r.floor(gaps=[(24, 28)])
    r.fill(24, 28, 29, 29, "^")
    r.platform(10, 22, 15, "B")          # a ceiling band with two traps hanging from it
    r.put(14, 16, "V")
    r.put(19, 16, "V")
    r.put(34, 26, "E")                   # the walker patrols the entry side, east of the pit
    r.put(31, 26, "C")
    r.put(6, 26, "K")
    r.put(2, 26, "X")
    return r.text("east: room_08\n")


if __name__ == "__main__":
    OUT.mkdir(exist_ok=True)
    for name, build in (("room_05", room_05), ("room_06", room_06), ("room_07", room_07),
                        ("room_08", room_08), ("room_09", room_09)):
        (OUT / f"{name}.txt").write_text(build())
        print("wrote", OUT / f"{name}.txt")
