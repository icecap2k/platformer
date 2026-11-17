# Platformer

A small 2D platformer written in **C++20** with **SDL3**, built to learn how to
structure a game with clear layers (in the spirit of DDD). Levels are screens
connected through their four edges and are authored with
[Tiled](https://www.mapeditor.org/).

The development diary is a Spanish blog in [`docs/`](docs/): one post per epic.

## Controls

| Action | Keys |
|---|---|
| Move | Arrow keys or `A` / `D` |
| Jump | `Space`, `Up` or `W` |
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

- **Tile layer**: any non-zero tile is solid. Tile 1 is ground, 2 is brick.
- **Map properties** `north`, `south`, `east`, `west` (string): id of the room
  behind that edge. No property means a wall. The id is the file name.
- **Objects** of class `coin` (rectangles): collectible coins.

## Third-party code

- [nlohmann/json](https://github.com/nlohmann/json) (MIT), in `game/third_party/nlohmann`
- [stb_image](https://github.com/nothings/stb) (public domain), in `game/third_party/stb`
- [doctest](https://github.com/doctest/doctest) (MIT), fetched by CMake
