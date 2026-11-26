---
title: Tiles y mapas
section: dominio
order: 2
summary: El mundo es una rejilla de casillas de 8 píxeles; qué es sólido y qué es rampa.
---

Una pantalla mide **320×240 píxeles** y es una rejilla de **40×30 casillas (tiles) de 8×8**. Cada casilla
guarda un número:

| Número | Significado |
|---|---|
| 0 | vacío |
| 1 | suelo (roca oscura) |
| 2, 3, 4 | otros bloques macizos |
| 5 | rampa que sube hacia la derecha |
| 6 | rampa que sube hacia la izquierda |

## `TileMap`

Guarda la rejilla en **una sola lista**, fila tras fila. La casilla (columna, fila) está en
`fila * ancho + columna`:

<!-- src: game/src/domain/world/tile_map.cpp -->
```cpp
int TileMap::tileAt(int tile_x, int tile_y) const {
    if (tile_x < 0 || tile_y < 0 || tile_x >= width_ || tile_y >= height_) {
        return 0;
    }
```

Fuera de la rejilla todo está vacío: así nadie tiene que comprobar los límites antes de preguntar.

## Sólido, rampa o suelo

El resto del juego no mira los números: pregunta por la **forma**.

<!-- src: game/src/domain/world/tile_map.h -->
```cpp
    // A whole block: collides on every side. Ramps are NOT solid in this sense.
    bool isSolid(int tile_x, int tile_y) const { return shape(tile_x, tile_y) == TileShape::Solid; }
    bool isSlope(int tile_x, int tile_y) const;
    // Something to stand on: a block or a ramp.
    bool isGround(int tile_x, int tile_y) const { return shape(tile_x, tile_y) != TileShape::Empty; }
```

- `isSolid`: bloquea por todos los lados (paredes, techos, suelo).
- `isGround`: se puede estar encima (bloque o rampa).

Al estar en **un solo sitio**, añadir un tipo de tile nuevo no obliga a buscar comparaciones por todo el código.

## La superficie de una rampa

A 45 grados, por cada píxel que avanzas subes uno:

<!-- src: game/src/domain/world/tile_map.cpp -->
```cpp
    // 45 degrees: the surface moves one pixel up for every pixel across.
    return tile_shape == TileShape::SlopeRight ? top + size - local : top + local;
```

`local` es la distancia desde el borde izquierdo de la casilla (entre 0 y 8).

Cómo se dibujan estas casillas: [Tiles en pantalla](../tiles-en-pantalla/). Cómo se mueve el jugador por
ellas: [Jugador y física](../jugador-y-fisica/).
