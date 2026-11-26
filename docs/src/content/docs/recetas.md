---
title: Recetas
section: trabajar
order: 2
summary: Cambios guiados para aprender tocando, de más fácil a más completo.
---

Después de cada receta: `cmake --build build && ./build/platformer`. Si no te gusta, `git restore .` descarta
los cambios sin guardar en git (**todos**, así que empieza con el repositorio limpio).

## 1. Saltar más alto

En `game/src/domain/physics/player_physics.h` cambia `kJumpSpeed`. Juega. Después ejecuta las pruebas: algunas
fallarán porque describen la altura anterior. Es su trabajo: avisarte de que has cambiado el comportamiento.

## 2. Cambiar un color

En `game/tools/art/make_orbs.py`, cambia los tonos de `COLOURS`, ejecuta el script y compila.

## 3. Mover una moneda

Abre `room_05.tmj` en Tiled, selecciona una moneda en la capa `objects`, arrástrala, guarda y ejecuta:

```bash
cmake --build build
./build/level_check game/assets/tiled room_01
```

## 4. Añadir un sonido

1. Escribe una función en `make_sounds.py` y añádela a `SOUNDS`.
2. Añade el valor al `enum Sound` y trátalo en `soundName` y `allSounds`.
3. Devuélvelo en `soundFor` para el evento que quieras.
4. Una prueba cuenta los sonidos: actualiza el número.

## 5. Un recogible nuevo (por ejemplo, una gema)

Es el recorrido completo por las capas. Sigue cómo está hecho el **orbe** (busca `Orb` en el código):

| Paso | Capa | Dónde |
|---|---|---|
| 1. El dato `Gem` | dominio | `domain/world/` y `Room` |
| 2. Detectar que se toca | dominio | `rules/coin_pickup` |
| 3. Estado y evento | aplicación | `GameSession`, `game_event.h` |
| 4. Leerlo de Tiled | infraestructura | `tiled_map_loader.cpp` |
| 5. Símbolo de texto | herramientas | `text_level` |
| 6. Comprobar que se alcanza | herramientas | `reachability`, `level_check` |
| 7. Dibujo y animación | presentación | script de arte, `animation`, `GameRenderer` |
| 8. Sonido | presentación | `sound_cues`, `make_sounds.py` |
| 9. Pruebas | | `tests/orb_test.cpp` como modelo |

Empieza por las pruebas del dominio y avanza hacia fuera. El compilador te ayudará: al añadir un evento, te
obligará a decidir su sonido.
