---
title: Herramientas y comprobador
section: niveles
order: 2
summary: Cómo level_check demuestra que un nivel se puede completar.
---

| Herramienta | Qué hace |
|---|---|
| `level_check` | comprueba que todo es alcanzable |
| `level_from_text` | texto → mapa de Tiled |
| `level_to_text` | mapa de Tiled → texto |

```bash
./build/level_check game/assets/tiled room_01
./build/level_from_text game/levels/room_05.txt game/assets/tiled/room_05.tmj
./build/level_to_text game/assets/tiled/room_02.tmj room_02.txt
```

## Cómo lo demuestra

Prueba **todo lo que se puede hacer**. Desde el punto de partida, en cada paso intenta las 6 combinaciones
(izquierda, derecha o nada × saltar o no) usando la **física real**, y apunta las situaciones nuevas. Es una
búsqueda en anchura, como explorar un laberinto:

<!-- src: game/tools/level_tools/reachability.cpp -->
```cpp
        for (const domain::InputState& input : inputs) {
            State next = current;
            domain::stepPlayer(next.player, input, room.map());
```

Un estado ya visto no se repite. Para que el número sea finito, posición y velocidad se **redondean**:

<!-- src: game/tools/level_tools/reachability.cpp -->
```cpp
    const auto x = static_cast<std::uint64_t>(std::llround(state.player.x * 2.0F) + 8192);
    const auto y = static_cast<std::uint64_t>(std::llround(state.player.y * 10.0F) + 8192);
```

Las rutas que tocan pinchos se descartan.

## Lo que NO comprueba

> Un "OK" no significa que el nivel sea bueno ni fácil.

- **Escarabajos** y **estalactitas trampa**: los ignora.
- **Agacharse y deslizarse**: nunca pulsa abajo.
- **El orden**: comprueba que cada cosa se alcanza desde el inicio, no que puedas ir de una a otra. Tras cambiar
  salas, lánzalo también desde la final:

```bash
./build/level_check game/assets/tiled room_09 20 190
```

- Tiene un límite de 20 millones de estados y hoy explora unos 16: un mundo más grande puede superarlo.

Es mucho más rápido si compilas en modo *Release* (`-DCMAKE_BUILD_TYPE=Release`).

Decisión: [ADR-0003](../decisiones/0003-comprobador-de-alcanzabilidad/).
