---
title: "Enemigos: algo que se mueve por su cuenta"
epic: "E12 · Enemigos"
date: 2026-11-19
summary: "El primer enemigo del juego: un escarabajo que patrulla, se pisa y reutiliza todo el sistema de daño. Qué decisiones se tomaron y qué se dejó fuera a propósito."
---

Hasta ahora todo lo peligroso estaba quieto: los pinchos esperan y la estalactita solo cae si te pones debajo. Un **enemigo** es lo primero que se mueve por su cuenta. La buena noticia es que las épicas anteriores ya habían dejado casi todo el terreno preparado.

## Lo que se reutilizó

- **Daño:** el enemigo no inventa nada. Tocarlo de lado llama a `hurtPlayer`, que llama a `applyDamage`: un corazón, retroceso, invulnerabilidad y el evento `PlayerHurt`. Lo había prometido el ADR-0005.
- **Dato de nivel + estado de partida:** el mismo patrón que monedas y estalactitas. `Enemy` es el dato (el rectángulo de Tiled es dónde empieza y su hitbox); `EnemyPatrols` guarda dónde está ahora y si sigue vivo, por sala e índice.
- **Eventos:** al pisarlo se emite `EnemyStomped` y `soundFor` lo traduce al sonido nuevo. Como `soundFor` usa `std::visit`, si me hubiera olvidado del sonido **no habría compilado**.

## Las decisiones

Las tres las eligió el autor:

1. **Se pisan.** Saltar encima lo elimina y te hace rebotar. Si mantienes el salto, rebotas alto; si lo sueltas, el recorte de salto de siempre lo deja en un saltito. Sin escribir código nuevo para eso.
2. **Camina y da la vuelta.** Va a 0,35 px/paso (el jugador, 1,5) y se gira ante una pared, una rampa o un borde sin suelo bajo el pie delantero. Esa última comprobación es lo que evita que se despeñe.
3. **`level_check` no los modela.** La búsqueda ya tarda unos 45 s; con un enemigo móvil cada estado dependería también de dónde están, y se dispararía. Se imprime cuántos hay y un aviso.

## ¿Qué es "pisar"?

Es la parte más delicada. Tocar a un enemigo desde arriba y de lado son el mismo solape de rectángulos; lo que los distingue es **cómo llegas**. La regla: vas **cayendo** y tus pies están a menos de 6 px de su parte alta. Si caminas hacia él (misma altura), o saltas hacia arriba y lo golpeas con la cabeza, te daña. Los 6 px existen porque una caída avanza hasta 6 px por paso: se detecta el pisotón con un paso de retraso, y sin margen se escaparía.

Pisar funciona incluso en plena invulnerabilidad, para que un golpe previo no te impida defenderte.

## Qué se dejó fuera

El enemigo está siempre pegado al suelo: **sin gravedad ni rampas**. Una rampa en su camino cuenta como pared. Tampoco hay animación al morir: desaparece. Está todo apuntado en el ADR-0014 y en el backlog.

## Tests

De 96 a **107 casos**: el patrullaje (velocidad, pared, borde, límite de sala, rampa, muerto), la regla de pisotón en sus cuatro variantes, la lectura de `walker` en Tiled y el símbolo `E` de ida y vuelta, el sonido, y la sesión: golpe de lado, pisotón con rebote, que un enemigo derrotado no vuelva a dañar y que el reinicio total lo resucite. Un test más comprueba que los enemigos de `room_03` y `room_04` **pisan suelo** y no flotan.

## Verlo

Una captura sin ventana confirmó que el escarabajo se dibuja y patrulla. Se ve **algo oscuro sobre el fondo de la cueva**; queda en el backlog afinar su color y el sonido `stomp` de oído, que no pude escuchar.

## Siguiente

Lo más lógico: **pantalla de título, pausa y Game Over**, o **checkpoints** si el reinicio total resulta duro ahora que hay algo que te quita corazones por sorpresa.
