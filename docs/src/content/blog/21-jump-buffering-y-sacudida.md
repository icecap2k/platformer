---
title: "Jump buffering y sacudida: que el juego responda mejor"
epic: "E15 · Jump buffering y sacudida"
date: 2026-11-23
summary: "Recordar un salto pulsado un poco antes de aterrizar y sacudir la pantalla al recibir un golpe. Y la lección de por qué el comprobador de niveles ignora el buffer."
---

Dos mejoras pequeñas que cambian mucho cómo se siente el juego.

## Jump buffering

Todos hemos pulsado salto **un instante antes de tocar el suelo** y que no pasara nada, porque el personaje aún estaba en el aire. El *jump buffering* lo arregla: una pulsación nueva que no se puede usar todavía se **recuerda 6 pasos (0,1 s)**; si en ese margen aterrizas, el salto sale en ese paso. Es el hermano del coyote time (saltar un poco *después* de dejar un borde).

Reglas que lo mantienen honesto:

- Sigue haciendo falta una **pulsación nueva**: mantener pulsado al aterrizar no salta.
- **No hay doble salto**: un salto gasta a la vez el buffer y el coyote.
- **Un golpe olvida la pulsación** pendiente.
- La ventana es corta a propósito: con 10 pasos o más empieza a saltar "solo", cuando ya no lo esperabas.

## Sacudida de pantalla

Al recibir un golpe, la imagen se desplaza unos píxeles enteros durante 14 pasos (0,23 s) con una amplitud que se apaga y un vaivén que alterna. No hay estado nuevo: sale de `invulnerable_steps`, que **ya cuenta hacia atrás desde el golpe**. Cuántos pasos han pasado desde el golpe es `total - invulnerable_steps`. Es otra función pura, como los demás efectos (ADR-0008). El renderer mueve el *viewport* y deja la tira descubierta en negro. Con la pausa no se aplica.

## La lección: el comprobador de niveles

Al meter `jump_buffer_steps` en el estado del jugador, `level_check` **dejó de terminar**: pasó de unos 9 millones de estados a superar el límite de 20 millones, porque cada estado en el aire se multiplicaba por el valor del contador.

La salida fue pensar qué hace realmente el buffer: **no alcanza nada que no se alcanzara ya**. Una pulsación recordada es equivalente a pulsar de nuevo justo en el paso del aterrizaje, y el comprobador ya prueba todas las pulsaciones en todos los pasos. Así que después de cada paso el comprobador pone el buffer a cero y todo vuelve a ser como antes: 9 millones de estados, mismo resultado. Es una simplificación que solo vale porque el buffer **solo adelanta** un salto, nunca da uno que no se pudiera dar.

También lo anoto como aviso: cualquier estado nuevo del jugador hay que decidir si entra en la clave de estados de `level_check` o si el comprobador puede ignorarlo por una razón como esta.

## Tests

De 132 a **142 casos**: el salto recordado sale al aterrizar, uno demasiado temprano se olvida, la ventana dura exactamente lo que dice, mantener pulsado no salta, no hay doble salto, un golpe borra la pulsación, y para la sacudida: no hay sin golpe, se apaga y no pasa de los 14 pasos, y alterna de signo.

## Lo que no pude comprobar

La sacudida solo la verifican los tests de su función: en una captura estática 12 pasos después del golpe no se aprecia. Hay que verla en el juego.

## Siguiente

Con los P1 prácticamente cerrados, lo natural es **más salas y una meta** (tarea 20), o las **partículas** (tarea 17).
