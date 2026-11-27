#!/usr/bin/env python3
"""Composes the cave music, game/assets/music/cave.wav (standard library only).

    python3 game/tools/audio/make_music.py            # writes the WAV file
    python3 game/tools/audio/make_music.py --report   # length and loudness, writes nothing

A calm 32 second loop in A minor, made of four layers:

    drone   a very low hum that slowly swells
    pad     soft chords (Am, F, Dm, E), each fading into the next
    bells   a few sparse notes, with the echo of the cave
    wind    a faint breath of filtered noise

It loops without a seam: everything that sounds past the end is added to the beginning (the
buffer is treated as a circle), and every slow wave fits a whole number of times in the loop.
The result is deterministic.
"""

import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_sounds import RATE, Noise, write_wav  # noqa: E402

OUT = Path(__file__).resolve().parents[2] / "assets" / "music" / "cave.wav"
SECONDS = 32
LENGTH = RATE * SECONDS
PEAK = 0.6

# Equal temperament, A4 = 440 Hz.
NOTES = {"D2": 73.42, "E2": 82.41, "F2": 87.31, "A2": 110.0, "C3": 130.81, "D3": 146.83, "E3": 164.81,
         "F3": 174.61, "G#3": 207.65, "A3": 220.0, "B3": 246.94, "C4": 261.63, "D4": 293.66, "A4": 440.0,
         "B4": 493.88, "C5": 523.25, "D5": 587.33, "E5": 659.25, "G5": 783.99, "A5": 880.0}

CHORDS = [  # one every 8 seconds
    ("A2", "E3", "A3", "C4"),
    ("F2", "C3", "F3", "A3"),
    ("D2", "D3", "F3", "A3"),
    ("E2", "E3", "G#3", "B3"),
]

BELLS = [  # (second, note, loudness)
    (1.0, "E5", 1.0), (2.5, "A4", 0.6), (5.5, "C5", 0.8), (9.0, "A5", 0.7), (10.5, "E5", 0.5),
    (13.0, "C5", 0.7), (17.5, "D5", 0.9), (19.0, "A4", 0.6), (22.0, "G5", 0.5), (25.0, "B4", 0.9),
    (27.5, "E5", 0.6), (29.5, "B4", 0.45),
]


def whole(hz):
    """The nearest frequency that completes a whole number of cycles in the loop."""
    return round(hz * SECONDS) / SECONDS


def add_note(buffer, start, duration, hz, gain, shape):
    """Adds a note, wrapping round the end of the loop. `shape(t, duration)` is its envelope."""
    first = int(start * RATE)
    step = 2 * math.pi * hz / RATE
    for i in range(int(duration * RATE)):
        buffer[(first + i) % LENGTH] += gain * shape(i / RATE, duration) * math.sin(step * i)


def swell(t, duration, edge=2.5):
    """Fades in and out slowly (a raised cosine at both ends)."""
    ramp = min(1.0, t / edge, (duration - t) / edge)
    return 0.5 - 0.5 * math.cos(math.pi * max(0.0, ramp))


def drone():
    buffer = [0.0] * LENGTH
    low, beat, fifth = whole(55.0), whole(55.25), whole(82.41)
    for i in range(LENGTH):
        t = i / RATE
        breathe = 0.75 + 0.25 * math.sin(2 * math.pi * 3 * t / SECONDS)
        buffer[i] = breathe * (0.16 * math.sin(2 * math.pi * low * t) + 0.10 * math.sin(2 * math.pi * beat * t)
                               + 0.05 * math.sin(2 * math.pi * fifth * t))
    return buffer


def pad():
    buffer = [0.0] * LENGTH
    for index, chord in enumerate(CHORDS):
        start = index * 8.0 - 1.25          # overlaps its neighbours while they cross fade
        for position, name in enumerate(chord):
            hz = NOTES[name]
            gain = 0.060 - 0.008 * position
            add_note(buffer, start % SECONDS, 10.5, hz, gain, swell)
            add_note(buffer, start % SECONDS, 10.5, hz * 1.004, gain * 0.6, swell)  # a gentle chorus
    return buffer


def bells():
    buffer = [0.0] * LENGTH

    def ring(t, duration):
        return min(1.0, t / 0.008) * math.exp(-3.2 * t) * (1.0 - t / duration)

    def shimmer(t, duration):
        return min(1.0, t / 0.004) * math.exp(-7.0 * t) * (1.0 - t / duration)

    echoes = ((0.0, 1.0), (0.37, 0.45), (0.74, 0.26), (1.13, 0.15), (1.57, 0.08))
    for second, name, loudness in BELLS:
        for delay, strength in echoes:
            add_note(buffer, second + delay, 2.2, NOTES[name], 0.13 * loudness * strength, ring)
            add_note(buffer, second + delay, 0.8, NOTES[name] * 2.76, 0.025 * loudness * strength, shimmer)
    return buffer


def wind():
    noise = Noise(97)
    extra = RATE * 2                        # rendered past the end, to cross fade into the start
    raw = [noise.next(0.035) for _ in range(LENGTH + extra)]
    for i in range(extra):
        k = i / extra
        raw[i] = raw[i] * k + raw[LENGTH + i] * (1.0 - k)
    out = []
    for i in range(LENGTH):
        gust = 0.55 + 0.45 * math.sin(2 * math.pi * 2 * i / LENGTH + 1.3)
        out.append(0.16 * gust * raw[i])
    return out


def render():
    layers = [drone(), pad(), bells(), wind()]
    mixed = [sum(values) for values in zip(*layers)]
    scale = PEAK / max(abs(v) for v in mixed)
    return [int(v * scale * 32767) for v in mixed]


if __name__ == "__main__":
    samples = render()
    rms = math.sqrt(sum(s * s for s in samples) / len(samples)) / 32767
    print(f"cave music   {len(samples) / RATE:5.2f} s   rms {rms:4.2f}   "
          f"peak {max(abs(s) for s in samples) / 32767:4.2f}   seam {abs(samples[0] - samples[-1])}")
    if "--report" not in sys.argv:
        OUT.parent.mkdir(parents=True, exist_ok=True)
        write_wav(OUT, samples)
        print(f"wrote {OUT}")
