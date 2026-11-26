---
title: "Checkpoints: perder sin empezar de cero"
epic: "E13 · Checkpoints"
date: 2026-11-23
summary: "El reinicio total era duro y ahora hay enemigos. Cómo se añadieron checkpoints sin tocar el modelo de salud, qué se conserva al reaparecer y por qué se reponen solo los enemigos de esa sala."
---

En el post 13 elegimos que, al quedarte sin corazones, **todo se reiniciara**. Dejamos escrito: "si resulta frustrante, subir los checkpoints". Con los enemigos del post 18 hay más formas de perder corazones por sorpresa, así que toca.

## Las decisiones

Las tomó el autor:

1. **Se reaparece en el último checkpoint** con salud completa.
2. **Un checkpoint es un objeto que se toca**, colocado en Tiled (clase `checkpoint`, símbolo `K` en texto). Así el diseñador decide dónde se "guarda".

## Cómo encaja

Casi todo el sitio ya estaba preparado:

- **Dato de nivel + estado de partida:** `Checkpoint` es dato (como `Coin`); cuál está activo es estado de `GameSession`, un `optional<(sala, índice)>`.
- **Un solo camino para morir:** `hurtPlayer` ya decidía qué pasa al llegar a 0 corazones. Ahora pregunta: ¿hay checkpoint? Si sí, `respawnAtCheckpoint()`; si no, `restart()` como siempre. Eventos nuevos: `CheckpointReached` y `PlayerRespawned`; `GameRestarted` queda para cuando no hay ninguno.
- **Sonido:** `soundFor` usa `std::visit`: al añadir los dos eventos el programa **no compilaba** hasta decidir su sonido. Reaparecer reutiliza el de reinicio y llegar a un checkpoint tiene uno nuevo.

## ¿Qué se conserva al reaparecer?

Es la parte con más matices:

- **Se conserva** lo ya conseguido: monedas, frutas y el resto del mundo (enemigos derrotados y estalactitas caídas en otras salas).
- **Se repone** lo de la sala donde reapareces: enemigos y estalactitas vuelven a su estado inicial (`resetRoom`). Si no, podrías reaparecer al lado de un escarabajo que ya venía a por ti, o bajo una estalactita a medias.

Sin ningún checkpoint, todo sigue como en el ADR-0005: reinicio total.

## Dibujo

Una linterna de cristal apagada que, al tocarla, se enciende en **turquesa** (distinta de las monedas cálidas y los enemigos violetas) con un halo que pulsa. Solo brilla el activo.

## Dos tropiezos con los tests

Mis primeros tests de muerte no morían: el jugador **cruzaba los pinchos** porque la invulnerabilidad de 1,5 s le daba tiempo a atravesarlos (y a entrar en otra sala, donde tocaba otro checkpoint, que además es correcto). Lo arreglé con una franja de pinchos más ancha y haciendo que el jugador se pare encima. Es un buen recordatorio de lo que ya hacía la invulnerabilidad.

## Tests

De 119 a **126 casos**: se anuncia una sola vez, reaparece con salud completa y en su sitio, conserva las monedas, repone los enemigos de su sala, y sin checkpoint reinicia todo. También el cargador de Tiled, el símbolo `K` de ida y vuelta, el sonido y que `level_check` exija poder tocarlos.

## Siguiente

La **pantalla de título y la pausa** (épica 14), que es donde viven las decisiones que quedan por encima de `GameSession`.
