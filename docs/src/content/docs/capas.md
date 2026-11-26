---
title: Las capas
section: arquitectura
order: 1
summary: Cuatro capas con una regla; las dependencias apuntan hacia las reglas del juego.
---

```
presentation ──► application ──► domain
      │                           ▲
      └──► infrastructure ────────┘
```

| Capa | Pregunta que responde | Ejemplos | ¿Usa SDL? |
|---|---|---|---|
| `domain` | ¿Cuáles son las reglas? | `Player`, `TileMap`, física, daño, enemigos | No |
| `application` | ¿Qué pasa en una partida? | `GameSession`, `GameFlow`, eventos | No |
| `infrastructure` | ¿Cómo hablo con el exterior? | ventana, teclado, audio, leer mapas | Sí (menos la carga de mapas) |
| `presentation` | ¿Cómo se ve y suena? | `GameRenderer`, animaciones | Solo el renderer y las texturas |
| `main.cpp` | ¿Cómo se conecta todo? | el bucle | Sí |

**La regla:** el dominio no conoce a nadie. No sabe que hay una pantalla, un teclado ni SDL.

Si vienes de **DDD**: el dominio son las entidades y reglas; la aplicación, los casos de uso; la
infraestructura, los adaptadores; y `main.cpp`, la raíz de composición.

## Por qué merece la pena

- **Se puede probar sin abrir una ventana.** Una prueba crea una sala, un jugador, simula 300 pasos y comprueba
  el resultado en milisegundos.
- **Las herramientas reutilizan el juego.** El [comprobador de niveles](../comprobador/) usa la misma física,
  así que no puede "creer" algo distinto de lo que hará el juego.
- **Cambiar lo de fuera no toca las reglas.** Un mando sería otro adaptador que produce el mismo `InputState`.

## Quién impone la regla

El sistema de compilación. Cada capa es una librería y solo puede usar lo que declara:

<!-- src: game/CMakeLists.txt -->
```cmake
add_library(application STATIC
    src/application/game_flow.cpp
    src/application/game_session.cpp)
target_link_libraries(application PUBLIC domain)
```

Si el dominio intentara incluir SDL, no compilaría. Además hay una librería aparte,
`presentation_animation`, con la parte de la presentación que es **lógica pura** (qué fotograma toca, qué
adorno va dónde), para poder probarla.

## La frontera en la práctica

La entrada es un buen ejemplo. El dominio define **qué quiere hacer el jugador**:

<!-- src: game/src/domain/input/input_state.h -->
```cpp
struct InputState {
    bool left = false;
    bool right = false;
    bool jump = false;
    bool down = false;  // crouch on firm ground; slide on a ramp
```

y la infraestructura decide **qué teclas** significan eso:

<!-- src: game/src/infrastructure/input/keyboard_input_mapper.cpp -->
```cpp
    input.left = keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A];
    input.right = keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D];
```

Sigue con [el bucle del juego](../bucle/). El porqué de esta organización está en el
[ADR-0001](../decisiones/0001-arquitectura-en-capas/).
