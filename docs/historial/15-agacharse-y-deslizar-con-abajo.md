---
title: "Agacharse y deslizar con la flecha abajo"
epic: "E9 · Rampas (corrección)"
date: 2026-11-19
summary: "Una malinterpretación corregida: deslizar se dispara con la flecha abajo, no al caminar. Y la misma tecla sirve para agacharse con el hitbox a la mitad."
---

En el post anterior el personaje se deslizaba al **caminar hacia la bajada** de una rampa. Al jugarlo, el autor lo vio al revés: se deslizaba cada vez que se movía a izquierda o derecha sobre la rampa, y lo que quería era deslizarse **al pulsar la flecha hacia abajo**. Había leído "hacia abajo" como una dirección de marcha cuando era una tecla.

Es un buen recordatorio de algo que vale en cualquier proyecto: cuando una frase tiene dos lecturas razonables, **conviene preguntar antes de implementar**, aunque la otra lectura parezca obvia.

## Una tecla nueva: abajo

Hasta ahora el juego conocía tres órdenes: izquierda, derecha y saltar. `InputState` gana una cuarta, `down`, y el adaptador de teclado la rellena con `Down` o `S`. Lo bonito de la arquitectura: el **dominio no sabe qué tecla es**; solo recibe "abajo".

Lo que hace depende de **dónde esté el jugador**:

| Situación | Abajo hace |
|---|---|
| Sobre una rampa | se **desliza** por la pendiente |
| En suelo firme | se **agacha** |
| En el aire | nada |

## Deslizar

El cambio es pequeño: antes se comprobaba "hay input hacia la bajada", ahora "se pulsa abajo". La dirección ya no la manda el jugador sino **la rampa**: se desliza por donde baja, aunque las flechas izquierda/derecha apunten a otro lado. Caminar por la rampa, subiendo o bajando, vuelve a ser una caminata normal. Todo lo demás se queda: velocidad, el impulso de 12 pasos al soltar o al acabar la rampa, y los motivos para cortarlo.

Además, un detalle que se descubrió probándolo: al salir por el pie de la rampa, el jugador quedaba **uno o dos pasos en el aire** (la superficie de la rampa estaba unos píxeles más alta que el suelo en el último paso) y la pose de salto parpadeaba en mitad del deslizamiento. Ahora, si acaba de dejar una rampa y hay suelo a menos de un tile, se pega a él.

## Agacharse

El hitbox pasa de 16 a **8 px de alto**, con **los pies donde estaban**: al agacharse la parte de arriba baja 8 px, no los pies. Agachado:

- Anda a la **mitad de la velocidad** (0,75 px/paso).
- **No puede saltar.**
- **Solo se levanta si hay hueco encima de la cabeza.** Si suelta la tecla bajo un techo bajo, sigue agachado hasta salir. Es justo lo que hará falta para pasar bajo algo o sobre una plataforma móvil con poco margen.
- En el aire no se agacha; si dejaba el suelo agachado, se levanta. Un golpe también lo levanta.

Un efecto simpático sin código extra: si llegas al suelo plano deslizándote y sigues pulsando abajo, te agachas **y sigues deslizándote** con el impulso (una barrida), porque el impulso no depende de la altura.

### El sprite sigue a los pies, no a la cabeza

El dibujo del personaje se colocaba a partir de la parte alta del hitbox. Con un hitbox que cambia de altura, eso hunde o eleva al personaje. Ahora se alinea **por los pies** (`parte baja del hitbox − alto del sprite`), y el sprite agachado (fotograma 16, uno solo, quieto o caminando) queda asentado en el suelo.

Esto trae una regla para el futuro: cualquier cosa nueva que mida o choque con el jugador debe usar `player.height` o `bounds()` y no suponer 16 px.

## El comprobador de niveles

`level_check` **no pulsa abajo**, así que no modela el agachado ni el deslizamiento. Es seguro en una dirección: un hitbox más pequeño y más velocidad solo ayudarían, así que las rutas que encuentra son reales. Pero **un nivel que *exija* agacharse se daría por inalcanzable** hasta que la búsqueda aprenda a hacerlo. Está en el backlog y en el ADR-0011.

## Tests

Pasamos de 71 a **80 casos**. Los de rampas se reescribieron para pulsar abajo, y hay nuevos (`crouch_test`) para la altura y los pies, la velocidad, que no se salta agachado, el paso bajo (se agacha, pasa, y se levanta al salir), la salida al aire y el golpe. En el camino hubo un test que fallaba **porque el jugador ya estaba en el suelo plano** cuando yo pensaba que seguía en la rampa: ahí abajo es agacharse, y es lo correcto.

## Siguiente

Quedan las **frutas que curan** y los **enemigos**. Cuando haya un paso bajo de verdad, habrá que enseñar a `level_check` a agacharse.
