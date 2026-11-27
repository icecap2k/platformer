#!/usr/bin/env python3
"""Synthesises the sound effects of game/assets/sounds/*.wav (standard library only).

    python3 game/tools/audio/make_sounds.py            # writes the WAV files
    python3 game/tools/audio/make_sounds.py --report   # prints each sound's length and loudness, writes nothing

Every sound is built from a few ingredients: oscillators (square, triangle, saw, sine),
pitch sweeps, low-passed noise and amplitude envelopes. The files are 22.05 kHz, mono,
16-bit PCM and deterministic (the noise uses a fixed seed), so regenerating gives the same
sounds. The names must match presentation::soundName (src/presentation/sound_cues.cpp).
"""

import math
import random
import struct
import sys
import wave
from pathlib import Path

RATE = 22050
OUT_DIR = Path(__file__).resolve().parents[2] / "assets" / "sounds"
PEAK = 0.8  # every sound is normalised to this fraction of full scale


# ---------------------------------------------------------------- ingredients

def square(phase, duty=0.5):
    return 1.0 if (phase % 1.0) < duty else -1.0


def triangle(phase):
    p = phase % 1.0
    return 4.0 * p - 1.0 if p < 0.5 else 3.0 - 4.0 * p


def saw(phase):
    return 2.0 * (phase % 1.0) - 1.0


def sine(phase):
    return math.sin(2.0 * math.pi * phase)


def sweep(start_hz, end_hz, t, duration):
    """Frequency at time t of an exponential glide from start_hz to end_hz."""
    return start_hz * (end_hz / start_hz) ** (t / duration)


def envelope(t, duration, attack=0.005, decay_power=2.0):
    """A fast attack, then a smooth decay to silence at `duration`."""
    if t < attack:
        return t / attack
    return max(0.0, 1.0 - (t - attack) / (duration - attack)) ** decay_power


class Noise:
    """Seeded white noise through a one-pole low-pass: `cutoff` 1.0 is raw noise, small is rumble."""

    def __init__(self, seed):
        self.rng = random.Random(seed)
        self.state = 0.0

    def next(self, cutoff=1.0):
        self.state += cutoff * (self.rng.uniform(-1.0, 1.0) - self.state)
        return self.state


def render(duration, voice):
    """Calls voice(t, duration) for every sample and returns the list of floats."""
    count = int(duration * RATE)
    return [voice(i / RATE, duration) for i in range(count)]


def mix(*tracks):
    length = max(len(t) for t in tracks)
    return [sum(t[i] if i < len(t) else 0.0 for t in tracks) for i in range(length)]


def delayed(track, seconds):
    return [0.0] * int(seconds * RATE) + track


# ---------------------------------------------------------------------- sounds

def jump():
    phase = [0.0]

    def voice(t, d):
        phase[0] += sweep(260, 640, t, d) / RATE
        return 0.55 * (0.7 * square(phase[0], 0.4) + 0.3 * triangle(phase[0])) * envelope(t, d, 0.004, 1.6)
    return render(0.17, voice)


def land():
    noise = Noise(11)
    phase = [0.0]

    def voice(t, d):
        phase[0] += sweep(120, 48, t, d) / RATE
        thud = sine(phase[0]) * envelope(t, d, 0.002, 2.5)
        puff = noise.next(0.18) * 1.6 * envelope(t, d, 0.001, 3.5)
        return 0.7 * thud + 0.4 * puff
    return render(0.13, voice)


def coin():
    def note(hz, d):
        phase = [0.0]

        def voice(t, dd):
            phase[0] += hz / RATE
            return 0.5 * square(phase[0], 0.5) * envelope(t, dd, 0.002, 1.3)
        return render(d, voice)
    return mix(note(988, 0.06), delayed(note(1319, 0.14), 0.06))


def fruit():
    notes = [523, 659, 784, 1046]  # C5 E5 G5 C6: a little rising sparkle
    tracks = []
    for index, hz in enumerate(notes):
        phase = [0.0]

        def voice(t, d, hz=hz, phase=phase):
            vibrato = 1.0 + 0.012 * math.sin(2 * math.pi * 7 * t)
            phase[0] += hz * vibrato / RATE
            return 0.45 * triangle(phase[0]) * envelope(t, d, 0.004, 1.5)
        tracks.append(delayed(render(0.14 if index < 3 else 0.2, voice), 0.075 * index))
    return mix(*tracks)


def stomp():
    """A squashed 'boing': a fast downward glide with a pop on top."""
    noise = Noise(31)
    phase = [0.0]

    def voice(t, d):
        phase[0] += sweep(520, 170, t, d) / RATE
        body = 0.7 * square(phase[0], 0.3) + 0.3 * triangle(phase[0])
        pop = noise.next(0.5) * envelope(t, d, 0.001, 6.0)
        return 0.5 * body * envelope(t, d, 0.003, 1.8) + 0.25 * pop
    return render(0.14, voice)


def checkpoint():
    """A bright, rising two-note chime: something just lit up."""
    tracks = []
    for index, hz in enumerate([523, 784]):
        phase = [0.0]

        def voice(t, d, hz=hz, phase=phase):
            phase[0] += hz / RATE
            return 0.5 * (0.6 * sine(phase[0]) + 0.4 * triangle(phase[0] * 2)) * envelope(t, d, 0.004, 1.6)
        tracks.append(delayed(render(0.3 if index else 0.18, voice), 0.12 * index))
    return mix(*tracks)


def orb():
    """A bubbly, magical rise: quick upward blips ending in a shimmering note."""
    tracks = []
    for index, hz in enumerate([392, 523, 659, 880]):
        phase = [0.0]

        def voice(t, d, hz=hz, phase=phase):
            phase[0] += sweep(hz, hz * 1.35, t, d) / RATE
            return 0.4 * sine(phase[0]) * envelope(t, d, 0.004, 1.4)
        tracks.append(delayed(render(0.1, voice), 0.06 * index))
    for hz in (1175, 1760):
        phase = [0.0]

        def shimmer(t, d, hz=hz, phase=phase):
            phase[0] += hz * (1.0 + 0.008 * math.sin(2 * math.pi * 9 * t)) / RATE
            return 0.25 * triangle(phase[0]) * envelope(t, d, 0.01, 1.5)
        tracks.append(delayed(render(0.45, shimmer), 0.24))
    return mix(*tracks)


def resonate(samples, hz, ring):
    """A two-pole resonator: what makes bursts of noise ring like struck iron (ring close to 1)."""
    a1, a2 = 2.0 * ring * math.cos(2.0 * math.pi * hz / RATE), -ring * ring
    y1 = y2 = 0.0
    out = []
    for x in samples:
        y = x * (1.0 - ring) + a1 * y1 + a2 * y2
        out.append(y)
        y1, y2 = y, y1
    return out


def gate():
    """Only the noise of an iron grate being hauled up, in step with its animation (it starts
    moving 0.35 s after the sound begins): a quick run of metallic rattles over a rough scrape."""
    moving, length = 0.35, 1.48
    rng = random.Random(73)
    strikes = [0.0] * int(length * RATE)
    at = moving
    while at < length - 0.12:                        # one short burst per rattle of the bars
        loud = rng.uniform(0.55, 1.0)
        for i in range(int(0.012 * RATE)):
            strikes[int(at * RATE) + i] += loud * rng.uniform(-1.0, 1.0) * (1.0 - i / (0.012 * RATE))
        at += rng.uniform(0.055, 0.085)
    scrape = Noise(79)

    def friction(t, d):
        if t < moving:
            return 0.0
        swell = min(1.0, (t - moving) / 0.1) * min(1.0, (d - t) / 0.12)
        chatter = 0.65 + 0.35 * math.sin(2 * math.pi * 23 * t)
        return scrape.next(0.55) * swell * chatter

    rubbing = render(length, friction)
    parts = [(resonate(strikes, 1130, 0.992), 5.0), (resonate(strikes, 1870, 0.990), 4.0),
             (resonate(strikes, 3050, 0.985), 2.5), (strikes, 0.25),
             (resonate(rubbing, 820, 0.93), 1.1), (resonate(rubbing, 2300, 0.90), 0.6)]
    return [sum(track[i] * gain for track, gain in parts) for i in range(len(strikes))]


def drip():
    """A water drop in the cave: a short "plip" that bends upwards, and its faint echo."""
    def plip(gain):
        phase = [0.0]

        def voice(t, d):
            phase[0] += sweep(620, 1500, t, d) / RATE
            return gain * sine(phase[0]) * envelope(t, d, 0.003, 2.2)
        return render(0.11, voice)
    return mix(plip(1.0), delayed(plip(0.35), 0.16), delayed(plip(0.14), 0.33))


def win():
    """A short fanfare: a rising arpeggio ending on a held chord note."""
    notes = [392, 494, 587, 784]  # G4 B4 D5 G5
    tracks = []
    for index, hz in enumerate(notes):
        phase = [0.0]
        last = index == len(notes) - 1

        def voice(t, d, hz=hz, phase=phase):
            phase[0] += hz / RATE
            body = 0.5 * square(phase[0], 0.5) + 0.5 * triangle(phase[0])
            return 0.45 * body * envelope(t, d, 0.006, 1.3)
        tracks.append(delayed(render(0.7 if last else 0.18, voice), 0.15 * index))
    return mix(*tracks)


def hurt():
    noise = Noise(23)
    phase = [0.0]

    def voice(t, d):
        phase[0] += sweep(540, 95, t, d) / RATE
        buzz = saw(phase[0]) * 0.6
        grit = noise.next(0.6) * 0.45
        return (buzz + grit) * envelope(t, d, 0.003, 1.4)
    return render(0.3, voice)


def slide():
    noise = Noise(37)

    def voice(t, d):
        cutoff = 0.08 + 0.32 * math.sin(math.pi * (t / d)) ** 2   # the whoosh opens, then closes
        swell = math.sin(math.pi * min(1.0, t / d)) ** 0.8
        return noise.next(cutoff) * 1.7 * swell
    return render(0.38, voice)


def stalactite_warning():
    noise = Noise(41)
    phase = [0.0]

    def voice(t, d):
        phase[0] += 58 / RATE
        tremolo = 0.55 + 0.45 * math.sin(2 * math.pi * 16 * t)       # a trembling rumble
        swell = (t / d) ** 0.7 * (1.0 - 0.15 * (t / d))
        rumble = noise.next(0.07) * 2.2 + 0.8 * sine(phase[0])
        return rumble * tremolo * swell
    return render(0.55, voice)


def stalactite_crack():
    noise = Noise(53)
    phase = [0.0]

    def voice(t, d):
        phase[0] += sweep(1400, 380, t, d) / RATE
        snap = square(phase[0], 0.3) * 0.35
        return (noise.next(0.9) * 0.8 + snap) * envelope(t, d, 0.0008, 3.0)
    return render(0.1, voice)


def stalactite_crash():
    noise = Noise(67)
    phase = [0.0]
    pebbles = Noise(71)

    def voice(t, d):
        phase[0] += sweep(95, 38, t, d) / RATE
        thud = sine(phase[0]) * envelope(t, d, 0.002, 1.8)
        burst = noise.next(0.35) * 1.2 * envelope(t, d, 0.001, 2.4)
        clicks = pebbles.next(0.95) * 0.5 * (1.0 if (int(t * 1000) % 47) < 3 and t > 0.08 else 0.0) * envelope(t, d, 0.0, 1.0)
        return 0.7 * thud + 0.6 * burst + clicks
    return render(0.4, voice)


def restart():
    notes = [330, 294, 262, 196]  # E4 D4 C4 G3: a sad little descent
    tracks = []
    for index, hz in enumerate(notes):
        phase = [0.0]
        last = index == len(notes) - 1

        def voice(t, d, hz=hz, phase=phase):
            phase[0] += hz / RATE
            body = 0.55 * square(phase[0], 0.5) + 0.45 * triangle(phase[0])
            return 0.5 * body * envelope(t, d, 0.006, 1.2)
        tracks.append(delayed(render(0.42 if last else 0.2, voice), 0.2 * index))
    return mix(*tracks)


SOUNDS = {
    "jump": jump,
    "land": land,
    "coin": coin,
    "fruit": fruit,
    "stomp": stomp,
    "checkpoint": checkpoint,
    "orb": orb,
    "gate": gate,
    "win": win,
    "drip": drip,
    "hurt": hurt,
    "slide": slide,
    "stalactite_warning": stalactite_warning,
    "stalactite_crack": stalactite_crack,
    "stalactite_crash": stalactite_crash,
    "restart": restart,
}


# ----------------------------------------------------------------------- output

def finish(samples):
    """Normalises to PEAK and fades the last milliseconds so the sound ends without a click."""
    peak = max(abs(s) for s in samples) or 1.0
    scale = PEAK / peak
    fade = int(0.006 * RATE)
    out = []
    for i, s in enumerate(samples):
        gain = scale * (min(1.0, (len(samples) - 1 - i) / fade) if i >= len(samples) - fade else 1.0)
        out.append(int(max(-1.0, min(1.0, s * gain)) * 32767))
    return out


def write_wav(path, samples):
    with wave.open(str(path), "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(RATE)
        wav.writeframes(struct.pack("<%dh" % len(samples), *samples))


if __name__ == "__main__":
    report = "--report" in sys.argv
    if not report:
        OUT_DIR.mkdir(parents=True, exist_ok=True)
    for name, make in SOUNDS.items():
        samples = finish(make())
        seconds = len(samples) / RATE
        rms = math.sqrt(sum(s * s for s in samples) / len(samples)) / 32767
        print(f"{name:20s} {seconds:5.2f} s   rms {rms:4.2f}   peak {max(abs(s) for s in samples) / 32767:4.2f}")
        if not report:
            write_wav(OUT_DIR / f"{name}.wav", samples)
    if not report:
        print(f"wrote {len(SOUNDS)} files to {OUT_DIR}")
