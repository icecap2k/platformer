# ADR-0008: Efectos visuales como funciones puras; paralaje descartado

- Estado: Aceptada
- Fecha: 2026-11-18

## Contexto
La decoración y las gotas de las estalactitas debían ser solo cosmética: sin tocar el dominio, sin guardar nada en los mapas, y reproducibles.

## Decisión
- Todo lo cosmético es una **función pura de la posición del tile y del reloj**, sin estado: `standingDecoration`/`hangingDecoration` (hash de la posición), `autotile` (máscara de lados expuestos) y `dripAt` (gotas).
- **Gotas:** cada estalactita de adorno tiene su propio ritmo: ciclo de 5 a 12 s con un desfase propio, un instante aleatorio de caída dentro del ciclo y aproximadamente **un ciclo de cada tres sin gota**. Nunca gotean todas a la vez. La gota se forma, cae con aceleración hasta el primer sólido que haya debajo (o el fondo de la pantalla) y salpica. No hay sonido ni efecto en el juego.
- La lógica visual pura vive en `presentation_animation` y se prueba sin SDL (incluido un test estadístico: cada estalactita activa poco tiempo, nunca todas a la vez, intervalos variables).
- El **fondo es una imagen fija** (`cave.png`).

## Alternativas descartadas
- **Fondo con paralaje de 3 capas** (se movía contra la posición del jugador): se implementó (módulo, tests y capas) y se probó; **al autor no le gustó y pidió dejarlo quieto**. Se eliminó por completo, incluido el código. No volver a proponerlo sin que lo pida.
- Estado por gota o decoración guardada en el nivel: complica sin aportar.

## Consecuencias
- El renderer es fino; cambiar el aspecto no toca el juego.
- Cualquier efecto nuevo debería seguir el mismo patrón (función de posición y reloj).
- **Luciérnagas** (E12): `fireflyRoutes(sala, mapa)` elige por hash 3 puntos de partida con la caja de vuelo (5×2 tiles) libre de suelo y `fireflyAt(ruta, tiempo)` da su posición. Ciclo: 3 s quietas aleteando → tramo de 5 tiles de lado y 2 hacia abajo en curva (empieza plano y acaba empinado, girando hacia donde va) → 3 s quietas → vuelta por la misma curva (sube). Cada una tiene su **desfase propio**, así no arrancan a la vez. Sprite `fireflies.png` (`make_fireflies.py`): aleteo × brillo, con halo translúcido.
