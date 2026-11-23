---
title: "Título y pausa: un estado por encima de la partida"
epic: "E14 · Título y pausa"
date: 2026-11-23
summary: "El juego arrancaba directamente jugando. Cómo añadir un título y una pausa sin que la partida sepa que existen los menús, y por qué Esc ya no cierra el juego."
---

Hasta ahora, al abrir el juego caías directamente en la sala 1 y `Esc` lo cerraba. Con checkpoints ya no hace falta una pantalla de Game Over, pero sí un **título** y una **pausa**.

## La idea: un estado por encima de `GameSession`

La partida (`GameSession`) sigue sin saber que existen los menús. Encima va **`GameFlow`**, que solo guarda en qué pantalla estás (`Title`, `Playing`, `Paused`), qué menú toca y qué opción está seleccionada. Recibe **órdenes de menú** sueltas (`MenuCommand`: arriba, abajo, aceptar, atrás, pausa) y devuelve `true` solo cuando hay que **empezar una partida nueva**.

Dos consecuencias que me gustan:

- **Elegir "Empezar" crea una `GameSession` nueva**, con el mundo recargado. No hay que acordarse de limpiar nada, porque no se reutiliza el estado viejo (monedas, checkpoint, enemigos...).
- **Pausar es no dar pasos.** El reloj sigue, pero el bucle no llama a `update`. Por eso se congela todo (estalactitas, enemigos) mientras las luciérnagas y el brillo de la fruta, que dependen del reloj, siguen moviéndose detrás del velo. Un detalle bonito y gratis.

## Entrada

Las teclas de juego son "mantenidas" (`InputState`); los menús necesitan **pulsaciones**: si mantienes ↓ no quieres bajar diez opciones. `KeyboardInputMapper` ahora entrega una lista de `MenuCommand` por orden, ignorando las repeticiones. Un futuro mando solo tendría que traducir sus botones a lo mismo.

Cambio visible: **`Esc` ya no cierra el juego**, pausa. Para salir: Esc en el título, "Quit" del menú o cerrar la ventana.

## El texto

No hay fuente propia. SDL3 trae una de **8×8** (`SDL_RenderDebugText`), la que ya usaba el contador de monedas. Para el título se amplía con `SDL_SetRenderScale`. Limitaciones: solo mayúsculas y sin acentos, así que los menús están en inglés. Una fuente propia queda en el backlog.

## Tests

De 126 a **132 casos**: el estado inicial, que solo Start pide una partida nueva, la selección que da la vuelta, salir desde el título, pausar y reanudar con tres teclas distintas, volver al título y empezar de cero, y salir desde la pausa. Como `GameFlow` no depende de SDL, todo se prueba sin ventana.

Una captura sin ventana del título y de la pausa sirvió para ajustar posiciones y colores.

## Siguiente

Lo que queda barato y de más valor: **CI** y acelerar `level_check`. Después, sonidos de menú y el mando.
