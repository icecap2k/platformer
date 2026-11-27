# Platformer

A small 2D platformer written in **C++20** with **SDL3**, built to learn how to
structure a game with clear layers (in the spirit of DDD). Levels are screens
connected through their four edges and are authored with
[Tiled](https://www.mapeditor.org/).

The documentation is a Spanish site in [`docs/`](docs/), written to be read and learnt from: architecture, domain,
graphics, animations, levels and recipes. Design decisions are recorded as ADRs in [`docs/adr/`](docs/adr/) and the
pending work is in [`docs/BACKLOG.md`](docs/BACKLOG.md).

## Controls

| Action | Keys |
|---|---|
| Move | Arrow keys or `A` / `D` |
| Jump | `Space`, `Up` or `W` (hold for a full jump, tap for a hop; press again to re-jump) |
| Crouch | `Down` or `S` on firm ground (half height, slow, no jumping; it only stands up when there is room) |
| Slide | `Down` or `S` while on a ramp (you slide down it; walking on a ramp does not slide) |
| Mute / unmute | `M` |
| Music volume | `-` and `+` |
| Pause / resume | `P` or `Esc` |
| Menus | `Up`/`Down` (or `W`/`S`), `Enter` or `Space` to pick; `Esc` on the title quits |

## Build and run

Requirements: CMake 3.20+, a C++20 compiler and SDL3 (`brew install sdl3`).

```bash
cmake -S game -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/platformer
```

The first configure downloads [doctest](https://github.com/doctest/doctest).
The build copies `game/assets/` next to the executable.

## Documentation

Requirements: Node and pnpm.

```bash
cd docs
pnpm install
pnpm dev      # http://localhost:4321
pnpm check    # the quoted code still exists and no link is broken
```

## Architecture

Dependencies only point towards the domain:

```
presentation ──► application ──► domain
      │                           ▲
      └──► infrastructure ────────┘
```

| Layer | Contains | SDL? |
|---|---|---|
| `domain` | player, physics, tile maps, rooms, rules (transitions, coins) | no |
| `application` | `GameSession`: one step of the game, emits events | no |
| `infrastructure` | window, textures, keyboard adapter, Tiled loader | yes |
| `presentation` | `GameRenderer`, animation choices | yes |
| `main.cpp` | composition root: wires everything | yes |

Everything except the SDL-facing code is covered by unit tests (`game/tests`).

## Levels

Rooms are Tiled maps (`game/assets/tiled/*.tmj`, JSON, CSV layer data):

- **Tile layer**: ids 1-4 are solid blocks (1 is ground, 2 is brick). Ids **5 and 6 are 45 degree ramps** (5 rises to
  the right, 6 to the left): not solid: you walk up and down them, and **slide down them** when you press down. A ramp must
  stand on solid tiles (filler under it and at its high side); there are no ceiling ramps.
- **Map properties** `north`, `south`, `east`, `west` (string): id of the room
  behind that edge. No property means a wall. The id is the file name.
- **Objects** of class `coin` (rectangles): collectible coins.
- **Objects** of class `fruit` (rectangles): a healing fruit, it gives back one heart (eaten even at full health).
- **Objects** of class `exit` (rectangles): the way out; touching it wins the game once every orb has been taken.
- **Objects** of class `orb` (rectangles, 16x16) with a string property `colour` (`red`, `green` or `yellow`): the orbs that open the exit.
- **Objects** of class `checkpoint` (rectangles): where the player comes back after losing every heart.
- **Objects** of class `walker` (rectangles): a patrolling enemy; the rectangle is where it starts and its hitbox. Put it standing on the floor.
- **Objects** of class `spikes` (rectangles): the rectangle is the hitbox. A hit costs one of the 3 hearts.
- **Objects** of class `falling_stalactite` (rectangles): a trap that shakes when the player walks under it,
  then falls. Hang it under a solid tile.
- `game/assets/tiled/platformer.tiled-project` defines these classes for Tiled's object *Class* field.

## Level tools

Built with the game (`build/level_check`, `build/level_from_text`, `build/level_to_text`). They link the
same `domain` library as the game, so they always use the real physics.

```bash
# Prove that every room and coin can be reached (also runs as a CTest test). It stops once
# everything is found; --full explores every situation anyway
./build/level_check game/assets/tiled room_01

# Draw a level as text, then turn it into a Tiled map the game loads
./build/level_from_text game/levels/example.txt example.tmj

# And the reverse: print an existing map as text
./build/level_to_text game/assets/tiled/room_02.tmj room_02.txt
```

The rooms are edited in Tiled (`game/assets/tiled/*.tmj` is the source). The text format is only a quick way to
sketch a new one: `game/levels/example.txt` (`.` empty, `#` ground, `B` brick, `3`/`4` tileset tiles 3 and 4, `C` coin, `F` healing fruit, `E` enemy (two tiles big, standing on the bottom of its tile), `R`/`G`/`Y` orbs (two tiles big), `K` checkpoint, `X` exit, `^` spikes (one and a half tiles tall, written in a trench in the floor), `V` stalactite trap, `/` and `\` ramps; header lines
`north/south/east/west: room_id`).

## Player art

The heroine (a small chunky figure with long white hair and a red clip, a black jacket over a grey t-shirt and
red trousers) is made by `game/tools/art/make_hero_sprites.py` (Python, standard library only) into
`game/assets/sprites/hero.png`: 32x28 cells, one column per pose (19: idle, an 8-frame run, rise, apex, fall, land, hurt,
slide, crouch) and one row per moment of the wave of her hair (4). The drawing is bigger than the 8x16
collision box.

```bash
python3 game/tools/art/make_hero_sprites.py
python3 game/tools/art/make_hero_sprites.py --preview preview.png   # enlarged, does not write the sheet
```

The coins, the HUD hearts and the healing fruit are in `game/assets/sprites/sprites.png`, drawn by
`game/tools/art/make_sprites.py` (the coin frames are preserved from the sheet on disk).

## Health and hazards

The player has 3 hearts. A hit (spikes, a falling stalactite) costs one, throws the player back with a small hop
and gives 1.5 s of invulnerability (the sprite blinks). With no hearts left the whole game restarts. Every hit goes
through `domain::applyDamage` (`domain/rules/damage.h`), so future enemies feel the same. `level_check` treats
touching spikes as deadly, so it only approves routes that avoid them (stalactites are not modelled).
The hazard sprites come from `game/tools/art/make_hazards.py`, the hearts from `make_sprites.py`.

The spikes are 16 px tall with a 12 px hitbox, usually placed in a one-tile trench dug into the floor.

## Checkpoints

A `checkpoint` object (text symbol `K`) lights up when touched and becomes where you come back after losing every
heart, with full health. Coins and fruits you took stay taken, and the enemies and stalactite traps of that room are
set up again. With no checkpoint reached, losing every heart restarts the whole game. Sprite:
`game/tools/art/make_checkpoints.py`.

## Winning

The cave has nine rooms and one way out, an `exit` object (text symbol `X`) in the last room. A portcullis
shuts it until the three **orbs** (red-orange, green and yellow, hidden in rooms 03, 06 and 08) have been taken;
walking into the last room with them raises it. Touching the open exit shows the ending screen with the coins
you collected. A taken orb leaves its dashed silhouette behind, and the counter at the top shows which are
missing. The sprites come from `game/tools/art/make_orbs.py` and `make_exit.py`.

`level_check` requires every orb and exit to be reachable from the start, but it does not check the order: after
changing rooms, also run it from the last one (`./build/level_check game/assets/tiled room_09 20 190`).

## Enemies

A `walker` (a cave beetle) paces along the floor and turns around at walls, ramps and ledges. Land on it from above
and it is defeated and you bounce (hold jump for a higher bounce); touch it any other way and it costs a heart like
any other hit. It also turns around before spikes, and a defeated one lies squashed for 1 s. They are session state (`EnemyPatrols`), come back after a total restart, and `level_check` does not
model them (it reports how many there are). The sprite comes from `game/tools/art/make_enemies.py`.

A few fireflies drift through every room (`presentation/firefly.h`): purely cosmetic, a pure function of the room
and the clock, drawn from `game/tools/art/make_fireflies.py`.

## Sound

Sound effects react to the game's events (jump, landing, coin, fruit, hit, slide, the three stages of a
stalactite trap, stomping an enemy, reaching a checkpoint, taking an orb, the portcullis rising, winning,
game restart): `presentation::soundFor` maps each `GameEvent` to a sound and
`infrastructure::AudioPlayer` plays `game/assets/sounds/<name>.wav`. The WAV files are synthesised by
`game/tools/audio/make_sounds.py` (edit it and rerun to change them). If there is no audio device the game
runs silently. Press `M` to mute. To hear one without the game: `afplay game/assets/sounds/coin.wav`.

A calm 32 second loop plays as cave music (`game/assets/music/cave.wav`, composed by
`game/tools/audio/make_music.py`); `-` and `+` change its volume and it is quieter while paused. The decorative
stalactites can be heard dripping.

```bash
python3 game/tools/audio/make_music.py             # writes the music
python3 game/tools/audio/make_sounds.py            # writes the WAV files
python3 game/tools/audio/make_sounds.py --report   # lengths and loudness, writes nothing
```

## Rock tiles

Solid tiles are cave rock, drawn by `game/tools/art/make_rock_tiles.py` into `game/assets/sprites/rocks.png`
(and the 4 palette tiles Tiled shows, `game/assets/tiled/tiles.png`). Each tile is chosen by which of its
four sides touch open air (`presentation/autotile.h`): platform ends become pointed tips that taper
underneath, tops are lit and undersides are jagged. The outline is part of the art, and collision is
still a full tile. Tile id 1 is darker bedrock; any other id is lighter rock.

```bash
python3 game/tools/art/make_rock_tiles.py
python3 game/tools/art/make_rock_tiles.py --preview preview.png   # a sample level, enlarged
```

## Cave scenery

Sixteen props (giant mushrooms, crystals, stalagmites, vines, roots, shelf mushrooms...) grow on the floor, hang
from the ceiling and stick out of the walls. They are drawn by `game/tools/art/make_cave_decor.py` into
`game/assets/sprites/decor.png`, with colours faded towards the background so they are never mistaken for hazards
or pickups. Where they go is a pure function of the room (`presentation/decoration.h`): they only use empty tiles,
rest on rock and keep clear of anything that matters.

The stalactite clusters drip: a drop forms at the tip, falls until the first solid tile below and splashes
(`presentation/drip.h`). Every cluster has its own rhythm, so they never drip together.

```bash
python3 game/tools/art/make_cave_decor.py
python3 game/tools/art/make_cave_decor.py --preview preview.png   # enlarged, does not write the sheet
```

## Title screen

The title picture, `game/assets/background/title.png` (960x720, three times the logical screen), is the author's own
drawing (`game/art_sources/title.jpg`), cut and resized by `game/tools/art/make_title_screen.py` (macOS, uses
`sips`).

## Cave background

The cave behind the level is a still picture, `game/assets/background/cave.png` (320x240): a dark gradient with
distant rocks, drawn by `game/tools/art/make_cave_background.py`. In front of it, behind the rock, big glowing
mushrooms pulse and shed spores (`presentation/backdrop.h`, drawn by `game/tools/art/make_glow_mushrooms.py`).

```bash
python3 game/tools/art/make_cave_background.py
python3 game/tools/art/make_cave_background.py --preview preview.png   # does not write the image
python3 game/tools/art/make_glow_mushrooms.py
```

## Third-party code

- [nlohmann/json](https://github.com/nlohmann/json) (MIT), in `game/third_party/nlohmann`
- [stb_image](https://github.com/nothings/stb) (public domain), in `game/third_party/stb`
- [doctest](https://github.com/doctest/doctest) (MIT), fetched by CMake
