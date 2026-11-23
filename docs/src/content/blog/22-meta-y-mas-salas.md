---
title: "Una meta y cinco salas más: ahora hay un final"
epic: "E16 · Meta y más salas"
date: 2026-11-23
summary: "Una salida que gana el juego, una pantalla de final y cinco salas nuevas. Y lo que aprendió el comprobador de niveles sobre las conexiones verticales."
---

Con checkpoints, título y pausa, lo que faltaba era un **destino**. Decisiones del autor: la meta es **llegar a una salida** (sin exigir monedas) y el mundo crece **cinco salas o más**.

## La salida

Otra pieza más del patrón de siempre: `Exit` es dato de nivel (clase `exit` en Tiled, símbolo `X` en texto) y `GameSession` mira si el jugador la toca. Al hacerlo marca `won()`, emite `GameWon{monedas, total}` y **se queda quieta**: `update` ya no avanza.

Encima, `GameFlow` (el estado de los menús del post 20) gana una pantalla más, `Won`. `main.cpp` la abre al ver el evento. Muestra "YOU ESCAPED" y las monedas recogidas; Esc o "Back to title" vuelven al título, y "Start" ya creaba una partida limpia. Suena un arpegio ascendente (`win`) y la salida es un **arco de piedra con luz** que pulsa.

## Cinco salas

En vez de dibujar cinco `.tmj` a mano, las genera un script (`make_rooms_05_09.py`) con unas pocas llamadas sobre una cuadrícula de 40×30: suelo, plataformas, un foso... Cada sala tiene su idea: **el pasillo** (foso con pinchos y un escarabajo), **la subida** (plataformas en zigzag), **la caverna** (estalactitas bajo una plataforma colgante), **la joroba** (una rampa a una meseta) y **la salida**. Hay siete escarabajos y 13 monedas en total.

## Lo que enseñó `level_check`

Mi primer intento dejó `room_07`, `room_08` y `room_09` **inalcanzables**. La causa: las conexiones norte y sur colocan al jugador en el extremo opuesto **con la misma `x`**. Como en `room_03`, la sala de destino necesita un **hueco en el suelo** justo ahí, y la plataforma desde la que saltas debe estar lo bastante alta para que salgas con velocidad de subida suficiente. Mi plataforma superior estaba a 32 px del techo y el jugador no ganaba altura para salir del hueco; subirla a 24 px lo arregló. Después, una moneda sobre una plataforma colgante no se podía tocar: faltaba un escalón.

Es justo para lo que se hizo el comprobador: encontró en segundos lo que habría tardado una tarde de probar a mano.

## Pegas

- **`level_check` ya explora 16 millones de estados**, y el límite es 20. La próxima sala puede obligar a podar la búsqueda (backlog 13).
- No modela escarabajos ni estalactitas, así que **la dificultad real solo se comprueba jugando**.

## Tests

De 142 a **148 casos**: la salida en Tiled y en texto (ida y vuelta), que gana una sola vez con las monedas recogidas y que luego la sesión se detiene, la pantalla de final del `GameFlow`, el sonido, y que el mundo tiene nueve salas, una sola salida y nada enterrado ni flotando.

## Siguiente

Con un final jugable, lo que queda son **partículas**, el **mando** y **guardar la partida**.
