---
title: "Resumen de la arquitectura y siguientes pasos"
epic: "E7 · Pulido (2/2)"
date: 2026-11-17
summary: "Qué hemos construido, cómo encaja todo, qué decisiones se repetirán en cualquier juego y por dónde seguir."
---

Tenemos un plataformas jugable: te mueves, saltas, recorres cuatro pantallas conectadas y recoges monedas, con sprites y animación. Este post es el mapa de todo.

## Las capas

```
presentation ──► application ──► domain
      │                           ▲
      └──► infrastructure ────────┘
```

| Capa | Qué contiene | ¿Usa SDL? |
|---|---|---|
| `domain` | `Player`, física, `TileMap`, `Room`, `RoomGraph`, `Coin`, reglas | no |
| `application` | `GameSession`, eventos (`RoomChanged`, `CoinCollected`) | no |
| `infrastructure` | `Window`, `Texture`, `KeyboardInputMapper`, cargador de Tiled | sí (el cargador no) |
| `presentation` | `GameRenderer`; `animation` (lógica pura) | sí (`animation` no) |
| `main.cpp` | raíz de composición: crea y conecta todo | sí |

**La regla**: las flechas apuntan hacia el dominio. El compilador la hace cumplir, porque `domain` y `application` ni siquiera enlazan con SDL: si alguien intenta usar SDL ahí, no compila.

## Qué se testea y qué no

Todo lo que no toca SDL tiene tests (28 casos, más de 250 aserciones):

- Física y colisiones, reglas de transición y de monedas.
- La sesión entera: simular una partida sin ventana.
- Los niveles reales: cargan bien, los enlaces son recíprocos, ninguna moneda está enterrada.
- La lógica de animación.

Lo que no tiene test automático es lo que *habla con SDL*: abrir la ventana, leer el teclado y dibujar. Esas piezas son deliberadamente **finas**: casi no deciden nada, solo traducen. Cuanta menos lógica en ellas, menos riesgo hay de que se rompan sin que lo notes.

## Las ideas que se repetirán en cualquier juego

| Idea | Dónde la vimos |
|---|---|
| **Paso fijo**: simular siempre en pasos iguales y variar cuántos por frame | E1, `FixedTimestep` |
| **Puertos y adaptadores**: el dominio habla de acciones, no de teclas | E2, `InputState` |
| **Datos vs. comportamiento**: structs simples y funciones que los mueven | E3, `Player` y `stepPlayer` |
| **Resolver los ejes por separado** en colisiones | E4 |
| **Datos del nivel vs. estado de la partida** | E5 y E6, `Room` vs. `CoinCollection` |
| **Constructores que validan**: un objeto creado siempre es coherente | `TileMap`, `RoomGraph` |
| **Eventos**: informar de lo ocurrido sin acoplar quién reacciona | E6 |
| **El tiempo y la aleatoriedad se pasan, no se leen**, para poder testear | `FixedTimestep`, `playerPose` |
| **Lo cosmético vive en presentación** | E7, dirección del sprite |

## Cómo añadir cosas (receta rápida)

- **Una sala nueva**: dibuja el mapa en Tiled, añade `north`/`south`/`east`/`west`, y añade su nombre a la lista de `loadWorld()` en `main.cpp`. El test de "enlaces recíprocos" te avisará si olvidas el enlace de vuelta (pero ojo: está escrito con la lista de 4 salas; añade la nueva).
- **Una moneda nueva**: un objeto rectángulo con `Class = coin` en la capa de objetos de la sala.
- **Un tipo de tile nuevo**: da un id al tile, decide en `TileMap::isSolid` qué hace y añade su dibujo en `tileSheetIndex`.
- **Un efecto al recoger la moneda** (sonido, partículas): reacciona al evento `CoinCollected` en `report()` de `main.cpp`. La lógica del juego no cambia.

## Por dónde seguir (siguientes épicas)

1. **Mando**: un `GamepadInputMapper` que produzca el mismo `InputState`. Es el beneficio directo de E2: el dominio no cambia. Habrá que decidir cómo combinar teclado y mando (el de "último que se usó", o los dos a la vez).
2. **Sonido**: reaccionar a `CoinCollected` y `RoomChanged` desde un `AudioPlayer` en infraestructura. Tu `jail_audio` de Coffee Crisis es una buena base.
3. **Enemigos**: nueva entidad en el dominio, con su paso de física y regla de colisión con el jugador. Habrá que crear un evento `PlayerHurt` y decidir qué ocurre al morir.
4. **Guardado de partida**: serializar `CoinCollection` y la sala actual. Recuerda la limitación de los índices de moneda (usar el `id` del objeto de Tiled).
5. **Pantalla de título y menús**: un estado superior al `GameSession` (por ejemplo, `Title`, `Playing`, `Paused`) en la capa de aplicación.
6. **Salto más fino**: exigir soltar y pulsar de nuevo, altura variable según cuánto mantienes, "coyote time" (permitir saltar unos milisegundos tras salir de una plataforma).
7. **Plataformas de un solo sentido, pinchos, puertas con llave**: gracias a que `isSolid` es el único sitio que decide qué es sólido, se añaden ahí sin tocar el resto.

## Lo que has aprendido, en una frase por tema

- **C++ moderno**: RAII, `std::variant`, `std::optional`, structured bindings, excepciones en constructores.
- **SDL3**: ventana, renderer con resolución lógica, texturas, teclado.
- **CMake**: librerías por capas, dependencias que se hacen cumplir, tests con `FetchContent`.
- **Diseño**: separar dominio de infraestructura es lo que hace posible testear un juego sin abrir una ventana.

Gracias por seguir el diario. Si quieres retomarlo, cada épica nueva puede abrir con un post como estos.
