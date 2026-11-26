---
title: "Rampas de 45° y deslizamiento"
epic: "E9 · Rampas"
date: 2026-11-19
summary: "Una rampa no es ni un bloque ni aire. Cómo enseñamos a la física a andar por una diagonal, por qué el jugador se apoya por el centro, y cómo se desliza al bajar."
---

La salida de `room_01` era una escalera: peldaños de un tile que obligaban a saltar cada vez. Esta épica la sustituye por una **rampa de 45°** que se sube andando y, al bajarla, **te hace resbalar**. Es el cambio de física más delicado hasta ahora, porque la colisión era muy simple: *un tile es sólido o no lo es*.

## El problema: una rampa no encaja en "sólido o no"

Si una rampa fuera sólida, el jugador chocaría con ella como con una pared. Si no fuera nada, la atravesaría. Necesitamos algo intermedio: **no bloquea, pero tiene superficie**. Por eso el mapa ahora distingue tres cosas:

| | `isSolid` | `isGround` | Qué es |
|---|---|---|---|
| Bloque (ids 1-4) | sí | sí | choca por todos los lados |
| **Rampa** (ids 5 y 6) | no | sí | se pisa, tiene una superficie inclinada |
| Aire | no | no | nada |

Lo bueno de este reparto es que **`isSolid` conserva su significado** ("bloque entero"), así que la colisión por cajas, que ya funcionaba y tiene muchos tests, no cambia. La rampa se añade **encima**, sin tocar lo anterior. Las gotas y la estalactita trampa pasan a preguntar `isGround`, porque tanto un bloque como una rampa los detienen.

La superficie de una rampa de 45° es una fórmula de una línea: sube un píxel por cada píxel que avanzas. Para la rampa que sube hacia la derecha, `y = (fila+1)·8 − x_local`.

## Apoyarse por el centro de los pies

Un hitbox es un rectángulo de 8 px de ancho; una diagonal, no. ¿En qué punto del rectángulo medimos "estar sobre la rampa"? Elegimos **el centro de los pies**. Después de moverse, si el centro está sobre una rampa y los pies están en la superficie o por debajo, colocamos al jugador exactamente en `superficie − altura`.

Dos detalles hacen que se sienta bien:

- **Al bajar, el jugador se pega a la rampa.** Si no, cada paso hacia abajo dejaría los pies un poco en el aire (la superficie baja más rápido que la gravedad) y bajaría a saltitos. Si ya estaba en el suelo, se le permite seguir la superficie hasta 8 px por debajo.
- **No se hace mientras sube.** Un salto abandona la rampa; no queremos pegarlo otra vez al suelo en el primer paso.

### El pie delantero y los bloques de relleno

Una rampa tiene bloques de **relleno** debajo y a su lado alto, para que se vea sólida. Al subir, el pie delantero queda **hundido hasta 4 px** en la diagonal (porque solo medimos el centro), y ahí asoma el borde de un bloque de relleno. Sin remedio, el relleno lo frenaría como si fuera una pared.

La solución: mientras el jugador está sobre una rampa, **el borde inferior del cuerpo (4,5 px) no cuenta para las paredes**. Una pared de verdad sigue frenando, porque ocupa el resto del cuerpo. Es una limitación conocida, anotada en el ADR-0010: el pie delantero se hunde un poco.

## Deslizarse

> **Corrección posterior (ver el post 15):** al probarlo resultó que el deslizamiento no debe dispararse *caminando* hacia la bajada, sino **pulsando la flecha abajo** sobre la rampa. Lo que sigue describe la primera versión; la geometría y el impulso no cambian.

Las reglas que decidió el autor:

- **Quieto sobre la rampa, no se mueve.** Nada de resbalar solo.
- **Subir es normal.**
- **Bajar andando desliza:** velocidad de 2,4 px/paso (1,6 veces la carrera).
- **Conserva el impulso** unos pasos al acabar la rampa o al saltar, y se va frenando hasta pararse (12 pasos).
- **Se corta** si pulsas al contrario, chocas con una pared o recibes un golpe.

En el código es un contador, `slide_steps`, junto a la dirección del deslizamiento. Mientras baja por la rampa se rellena al máximo; fuera de ella baja de uno en uno y la velocidad es `2,4 × pasos/12`. Una ventaja: **el salto desde un deslizamiento conserva el impulso en el aire** sin ningún código extra, porque el contador no depende de estar en el suelo.

Mientras hay deslizamiento en el suelo, el personaje usa una **pose nueva** (agachado, echado hacia atrás, con una pierna estirada). Pasamos de 15 a 16 fotogramas.

## El dibujo de la rampa

Una rampa es un triángulo. Para que el contorno quede bien, necesitaba saber qué lados de sus vecinos están tapados por ella: una rampa que sube a la derecha rellena su cara **sur y este**, no la norte ni la oeste. Por eso el mapa tiene `blocksFace(col, fila, lado)`, y el renderer lo usa para calcular qué contornos hay que dibujar. Así el bloque de relleno de su lado alto no dibuja un borde donde toca la rampa, y la meseta enlaza sin costuras.

La hipotenusa lleva contorno oscuro y una línea clara justo debajo, como la cara superior de las rocas.

## Lo que se rompió por el camino

Dos tests míos fallaban y no era la física:

- Un test hacía correr al personaje cuando ya no había suelo **porque salía del mapa por el borde** (en el juego `confineToMap` lo impide, en un test no) y caía al vacío. Ahora los tests se detienen al pasar el pie de la rampa.
- Al escribir el espejo (la rampa que sube hacia la izquierda), equivoqué cuál era el lado alto en el test. La rampa estaba bien; el test, no.

## El comprobador de niveles

`level_check` usa la misma física que el juego, así que **heredó las rampas sin tocar su lógica**. Lo único que cambió es su clave de estado, que ahora incluye el deslizamiento (cuatro bits para el contador y uno para la dirección). Aprueba las 4 salas y las 3 monedas con la rampa nueva; tarda un minuto, unos 12 millones de estados.

## Tests

Pasamos de 57 a **71 casos**. Los de rampas usan la **geometría real de `room_01`** (rampa de 5 tiles, relleno y meseta) y su espejo: subir sin que los bloques lo frenen y sin dar saltos, bajar sin quedarse en el aire, quedarse quieto, caer encima y aterrizar en la superficie, saltar, deslizarse, conservar el impulso y cancelarlo, y una pared al final de la rampa.

## Siguiente

Quedan las **frutas que curan** y los **enemigos**; estos últimos tendrán que saber andar por rampas. Los pinchos sobre una rampa y las rampas de techo siguen sin soporte (están en el backlog).
