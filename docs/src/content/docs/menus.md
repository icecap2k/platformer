---
title: Pantallas y menús (GameFlow)
section: aplicacion
order: 2
summary: Título, juego, pausa y final, como una máquina de estados por encima de la partida.
---

`GameSession` no sabe que existen menús. Por encima está `GameFlow`, que decide **qué pantalla se ve** y **si
la partida avanza** (solo en `Playing`).

```
          Start                   P / Esc
 Title ───────────► Playing ◄───────────────► Paused
   ▲                   │                        │
   │                   │ se gana                │ Back to title
   │                   ▼                        │
   ├───────────────── Won                       │
   └────────────────────────────────────────────┘
```

## Comandos, no teclas

Los menús reciben `MenuCommand` (`Up`, `Down`, `Confirm`, `Back`, `Pause`), **una vez por pulsación**. Es
distinto de `InputState`, que dice qué teclas están mantenidas: en un menú no quieres que mantener la flecha
recorra las opciones a toda velocidad.

<!-- src: game/src/application/game_flow.cpp -->
```cpp
    switch (command) {
        case MenuCommand::Up:
            selected_ = (selected_ + count - 1) % count;
            return false;
        case MenuCommand::Down:
            selected_ = (selected_ + 1) % count;
            return false;
```

`% count` (el resto de la división) hace que la selección dé la vuelta. Sumar `count - 1` en vez de restar 1
evita números negativos.

## Empezar de cero

`handle` devuelve `true` cuando hay que crear una partida nueva, y `main` lo hace:

<!-- src: game/src/main.cpp -->
```cpp
                if (flow.handle(command)) {
                    session.emplace(loadWorld(), "room_01", kSpawnX, kSpawnY);  // a fresh game
                }
```

Tirar la partida anterior y construir otra es la forma más segura de no arrastrar restos.

Decisión: [ADR-0016](../decisiones/0016-pantalla-de-titulo-y-pausa/).
