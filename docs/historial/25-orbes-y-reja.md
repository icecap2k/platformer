---
title: "Tres orbes y una reja que se levanta"
epic: "E19 · Orbes"
date: 2026-11-26
summary: "Un coleccionable con volumen, burbujas y latido que abre la salida, y la reja que sube con su ruido de hierro."
---

La salida ya no se abre sola: hacen falta **tres orbes** (rojo-naranja, verde y amarillo) repartidos por la cueva.

## Dibujar una esfera

`make_orbs.py` no pinta la bola a mano. Para cada píxel calcula la **normal** de una esfera real y cuánto mira hacia la luz: de ahí salen las bandas de color. Encima, un brillo nítido arriba a la izquierda, un reflejo tenue en el borde opuesto y un halo. El latido es el radio variando con un seno, y las burbujas son pequeños sellos que suben y se bambolean, recortados por el cristal. Doce fotogramas que cierran el bucle.

Al coger un orbe queda su **silueta a trazos**, y el marcador de arriba muestra cuáles faltan.

## El mismo patrón, otra vez

`Orb` es dato de nivel (`R`, `G`, `Y` en texto, clase `orb` en Tiled) y cuáles se han cogido es estado de la partida, igual que las monedas. La diferencia está en una función: `exitOpen()`.

## La reja

Con los tres, al entrar en la última sala el rastrillo **sube**. La animación es una función pura del tiempo que lleva en pantalla (`exitSpriteFrame`), y el sonido llega por un evento, `GateOpened`: el juego dice *qué pasó* y la presentación decide cómo se ve y cómo suena. La primera versión del sonido tenía golpes y un retumbar; quedó solo el hierro, hecho con ráfagas de ruido que pasan por **resonadores**, que es lo que le da el timbre metálico.

## Ojo al comprobador

`level_check` exige poder tocar los tres, pero no sabe de **orden**: si una sala fuera de un solo sentido, podrías quedarte sin volver. Lo comprobé lanzándolo también desde la sala final.
