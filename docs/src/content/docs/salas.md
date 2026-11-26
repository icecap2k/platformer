---
title: Salas y transiciones
section: dominio
order: 5
summary: El mundo como un grafo de pantallas, y qué pasa al cruzar un borde.
---

## `Room` y `RoomGraph`

Una `Room` es una pantalla: su `TileMap`, las salas vecinas y las listas de lo que contiene. `RoomGraph` es el
mundo entero y **comprueba al construirse** que todo encaja:

<!-- src: game/src/domain/world/room_graph.cpp -->
```cpp
    for (const auto& [id, room] : rooms_) {
        for (const auto& [side, neighbor_id] : room.neighbors()) {
            if (!contains(neighbor_id)) {
                throw std::invalid_argument("Room '" + id + "' links to unknown room '" +
                                            neighbor_id + "'");
            }
        }
    }
```

Un mapa mal enlazado falla al arrancar, no cuando el jugador llega allí.

## El mundo actual

```
                 room_09 (salida) ── room_08 ── room_07
                                                   │
          room_03                                  │
             │                                     │
room_01 ── room_02 ── room_04 ── room_05 ──────── room_06
```

| Sala | Vecinas | Orbe |
|---|---|---|
| room_01 | este: 02 | |
| room_02 | oeste: 01, norte: 03, este: 04 | |
| room_03 | sur: 02 | rojo |
| room_04 | oeste: 02, este: 05 | |
| room_05 | oeste: 04, este: 06 | |
| room_06 | oeste: 05, norte: 07 | verde |
| room_07 | sur: 06, oeste: 08 | |
| room_08 | este: 07, oeste: 09 | amarillo |
| room_09 | este: 08 | (salida) |

## Cruzar un borde

Son tres reglas pequeñas, en `rules/room_transition`:

1. **`confineToMap`**: un borde sin vecino es una pared.
2. **`exitedThrough`**: ¿el **centro** del jugador ha cruzado un borde? Con el centro, la sala cambia cuando
   está medio fuera.
3. **`enterRoom`**: aparece en el borde opuesto de la sala nueva, **conservando la velocidad**:

<!-- src: game/src/domain/rules/room_transition.cpp -->
```cpp
        case Side::East:  // appears on the west edge of the new room
            player.x = 0.0F;
            player.y = std::clamp(player.y, 0.0F, max_y);
            break;
```

La otra coordenada se mantiene. Esto condiciona el diseño de niveles: una conexión vertical necesita un hueco
en el suelo **en la misma `x`** de la sala de destino.
