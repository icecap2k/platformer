# Platformer

A small 2D platformer written in **C++20** with **SDL3**, built to learn how to
structure a game with clear layers (in the spirit of DDD). Levels are screens
connected through their four edges and are authored with
[Tiled](https://www.mapeditor.org/).

The development diary is a Spanish blog in [`docs/`](docs/): one post per epic. Design decisions are recorded as
ADRs in [`docs/adr/`](docs/adr/) (start there for context) and the pending work is in [`docs/BACKLOG.md`](docs/BACKLOG.md).

## Controls

| Action | Keys |
|---|---|
| Move | Arrow keys or `A` / `D` |
| Jump | `Space`, `Up` or `W` (hold for a full jump, tap for a hop; press again to re-jump) |
| Crouch | `Down` or `S` on firm ground (half height, slow, no jumping; it only stands up when there is room) |
| Slide | `Down` or `S` while on a ramp (you slide down it; walking on a ramp does not slide) |
| Mute / unmute | `M` |
| Quit | `Esc` |

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

## Blog

Requirements: Node and pnpm.

```bash
cd docs
pnpm install
pnpm dev      # http://localhost:4321
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
- **Objects** of class `checkpoint` (rectangles): where the player comes back after losing every heart.
- **Objects** of class `walker` (rectangles): a patrolling enemy; the rectangle is where it starts and its hitbox. Put it standing on the floor.
- **Objects** of class `spikes` (rectangles): the rectangle is the hitbox. A hit costs one of the 3 hearts.
- **Objects** of class `falling_stalactite` (rectangles): a trap that shakes when the player walks under it,
  then falls. Hang it under a solid tile.
- `game/assets/tiled/platformer.tiled-project` defines these three classes for Tiled's object *Class* field.

## Level tools

Built with the game (`build/level_check`, `build/level_from_text`, `build/level_to_text`). They link the
same `domain` library as the game, so they always use the real physics.

```bash
# Prove that every room and coin can be reached (also runs as a CTest test)
./build/level_check game/assets/tiled room_01

# Draw a level as text, then turn it into a Tiled map the game loads
./build/level_from_text game/levels/example.txt game/assets/tiled/room_05.tmj

# And the reverse: print an existing map as text
./build/level_to_text game/assets/tiled/room_02.tmj room_02.txt
```

Text format: `game/levels/example.txt` (`.` empty, `#` ground, `B` brick, `3`/`4` tileset tiles 3 and 4, `C` coin, `F` healing fruit, `E` enemy, `K` checkpoint, `^` spikes, `V` stalactite trap, `/` and `\` ramps; header lines
`north/south/east/west: room_id`).

## Player art

The player frames (14: idle, run, rise, apex, fall, land) are drawn in code by
`game/tools/art/make_player_sprites.py` (Python, standard library only). Edit the
frame definitions and run it to regenerate `game/assets/sprites/sprites.png`; the
coin frames in the same sheet are preserved.

```bash
python3 game/tools/art/make_player_sprites.py
python3 game/tools/art/make_player_sprites.py --preview preview.png   # enlarged, does not touch the sheet
```

## Health and hazards

The player has 3 hearts. A hit (spikes, a falling stalactite) costs one, throws the player back with a small hop
and gives 1.5 s of invulnerability (the sprite blinks). With no hearts left the whole game restarts. Every hit goes
through `domain::applyDamage` (`domain/rules/damage.h`), so future enemies feel the same. `level_check` treats
touching spikes as deadly, so it only approves routes that avoid them (stalactites are not modelled).
The hazard sprites come from `game/tools/art/make_hazards.py`, the hearts and the hurt frame from
`make_player_sprites.py`.

## Checkpoints

A `checkpoint` object (text symbol `K`) lights up when touched and becomes where you come back after losing every
heart, with full health. Coins and fruits you took stay taken, and the enemies and stalactite traps of that room are
set up again. With no checkpoint reached, losing every heart restarts the whole game. Sprite:
`game/tools/art/make_checkpoints.py`.

## Enemies

A `walker` (a cave beetle) paces along the floor and turns around at walls, ramps and ledges. Land on it from above
and it is defeated and you bounce (hold jump for a higher bounce); touch it any other way and it costs a heart like
any other hit. It also turns around before spikes, and a defeated one lies squashed for 1 s. They are session state (`EnemyPatrols`), come back after a total restart, and `level_check` does not
model them (it reports how many there are). The sprite comes from `game/tools/art/make_enemies.py`.

A few fireflies drift through every room (`presentation/firefly.h`): purely cosmetic, a pure function of the room
and the clock, drawn from `game/tools/art/make_fireflies.py`.

## Sound

Sound effects react to the game's events (jump, landing, coin, fruit, hit, slide, the three stages of a
stalactite trap, stomping an enemy, reaching a checkpoint, game restart): `presentation::soundFor` maps each `GameEvent` to a sound and
`infrastructure::AudioPlayer` plays `game/assets/sounds/<name>.wav`. The WAV files are synthesised by
`game/tools/audio/make_sounds.py` (edit it and rerun to change them). If there is no audio device the game
runs silently. Press `M` to mute. To hear one without the game: `afplay game/assets/sounds/coin.wav`.

```bash
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

The small things growing on the rocks (glowing mushrooms, crystals, pebbles) and hanging from them
(a stalactite, glowing roots) are `game/assets/sprites/decor.png`, drawn by
`game/tools/art/make_cave_decor.py`. Which one appears where is a hash of the tile position, so a room
always looks the same.

Stalactites drip: a drop forms at the tip, falls until the first solid tile below and splashes
(`presentation/drip.h`). It is only decoration, a function of the tile position and the clock: every
stalactite has its own rhythm (a drop every 5-12 s, some cycles skipped), so they never drip together.

```bash
python3 game/tools/art/make_cave_decor.py
python3 game/tools/art/make_cave_decor.py --preview preview.png   # enlarged, does not write the sheet
```

## Cave background

The still cave behind the level is `game/assets/background/cave.png` (320x240), drawn by
`game/tools/art/make_cave_background.py`. It is painted in three layers (far, middle, near) that
are saved as one composite.

```bash
python3 game/tools/art/make_cave_background.py
python3 game/tools/art/make_cave_background.py --preview preview.png   # does not write the image
```

## Third-party code

- [nlohmann/json](https://github.com/nlohmann/json) (MIT), in `game/third_party/nlohmann`
- [stb_image](https://github.com/nothings/stb) (public domain), in `game/third_party/stb`
- [doctest](https://github.com/doctest/doctest) (MIT), fetched by CMake
