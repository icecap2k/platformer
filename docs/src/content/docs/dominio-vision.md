---
title: Datos y estado
section: dominio
order: 1
summary: La idea que se repite en todo el dominio, y un mapa de sus piezas.
---

## La idea clave

El dominio separa siempre dos cosas:

| | **Datos del nivel** | **Estado de la partida** |
|---|---|---|
| Qué es | lo que dibujó quien hizo el nivel | lo que ha ocurrido jugando |
| ¿Cambia? | nunca | constantemente |
| Dónde vive | `Room` | `GameSession` |
| Ejemplo | "hay una moneda en (40, 208)" | "esa moneda ya está cogida" |

| Dato | Su estado |
|---|---|
| `Coin`, `Fruit`, `Orb` | `PickupCollection` |
| `Enemy` | `EnemyState` (en `EnemyPatrols`) |
| `FallingStalactite` | `StalactiteState` (en `StalactiteTraps`) |
| `Checkpoint` | cuál fue el último tocado |
| `Hazard`, `Exit` | no tienen estado propio |

Ventajas: reiniciar la partida es **borrar el estado**; el nivel no se estropea nunca; y guardar partida (aún
sin hacer) sería guardar solo el estado.

Cada elemento se identifica por **sala + posición en su lista**:

<!-- src: game/src/domain/world/coin_collection.cpp -->
```cpp
bool PickupCollection::isCollected(const RoomId& room, std::size_t index) const {
    return collected_.count({room, index}) != 0;
}
```

## El mapa del dominio

| Carpeta | Contiene | Página |
|---|---|---|
| `entities/` | `Player` | [Jugador y física](../jugador-y-fisica/) |
| `physics/` | `AABB`, el movimiento | [Jugador y física](../jugador-y-fisica/), [Rampas y agacharse](../rampas-y-agacharse/) |
| `world/` | mapas, salas, objetos | [Tiles y mapas](../tiles-y-mapas/), [Salas](../salas/), [Objetos](../objetos/) |
| `rules/` | daño, curación, recoger, cambiar de sala | [Salud y peligros](../salud-y-peligros/) |
| `enemies/` | patrulla y pisotón | [Enemigos](../enemigos/) |
| `input/` | `InputState`, `MenuCommand` | [Las capas](../capas/) |
| `time/` | `FixedTimestep` | [El bucle](../bucle/) |

## La pieza más pequeña: `AABB`

Casi todo choca o se toca usando rectángulos que no giran (*axis-aligned bounding box*). `x`, `y` son la
esquina superior izquierda y **la `y` crece hacia abajo**, como en la pantalla.

<!-- src: game/src/domain/physics/aabb.h -->
```cpp
    // Boxes that only touch along an edge do NOT overlap.
    bool overlaps(const AABB& other) const {
        return left() < other.right() && right() > other.left() && top() < other.bottom() &&
               bottom() > other.top();
    }
```

Dos rectángulos se solapan si se solapan en horizontal **y** en vertical. Tocarse por un borde no cuenta: así
un jugador de pie sobre el suelo no "choca" con él.
