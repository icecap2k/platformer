# ADR-0011: Agacharse y deslizar con la flecha abajo

- Estado: Aceptada
- Fecha: 2026-11-19
- Sustituye en parte al ADR-0010: lo que cambia es **qué dispara el deslizamiento**; la geometría de las rampas no cambia.

## Contexto
En el ADR-0010 se decidió que el personaje se deslizara "al bajar andando" por una rampa. Al probarlo, el autor vio que se deslizaba **al moverse a izquierda o derecha** sobre la rampa, y aclaró que quería deslizarse **al pulsar la flecha hacia abajo**. Fue una interpretación equivocada de "hacia abajo": era la tecla, no la dirección de la marcha. De paso se pidió **agacharse** en suelo firme, con el hitbox a la mitad, por si más adelante hay que pasar por un hueco bajo (por ejemplo, sobre una plataforma móvil).

## Decisión
Una tecla nueva, **abajo** (`InputState::down`; en el teclado, `Down` o `S`), con dos usos que dependen de dónde esté el jugador:

- **Sobre una rampa → desliza.** Pulsar abajo lo lanza por la pendiente **hacia la bajada, hacia donde apunten las flechas izquierda/derecha** (la dirección la manda la rampa). Caminar por la rampa, subiendo o bajando, es una caminata normal; quieto no se mueve. Se mantiene todo lo demás del ADR-0010: velocidad `kSlideSpeed`, impulso de 12 pasos que decae al soltar la tecla o al acabar la rampa, y lo cortan pulsar al contrario, una pared o un golpe.
- **En suelo firme (no en rampa) → se agacha.** El hitbox pasa de 16 a **8 px** (`kStandingHeight`, `kCrouchHeight`) **con los pies donde estaban** (`Player::crouching`). Agachado camina a `kCrouchSpeed` = 0,75 px/paso (la mitad de la carrera) y **no puede saltar**. Para levantarse hace falta **hueco encima de la cabeza** (`canStandUp`): en un paso bajo sigue agachado aunque suelte la tecla, y se levanta al salir. En el aire no se agacha; si dejaba el suelo agachado, se levanta. Un golpe también lo levanta.
- Si se pulsa abajo al llegar al suelo plano con impulso de deslizamiento, se agacha **y sigue deslizándose** con la velocidad que traía (una barrida). No hay que hacer nada especial: el impulso no depende de la altura.
- **Dibujo:** pose `Crouch` (fotograma 16; el jugador pasa a 17 fotogramas), con prioridad por debajo de `Hurt` y `Slide`. El sprite se alinea **por los pies** (`box.bottom() − alto del sprite + 1`) y no por la parte alta del hitbox, que ahora varía.
- **Pegado al suelo:** al salir del pie de una rampa el jugador se pega al suelo plano si está a menos de `kSlopeSnap` (si no, caía 1-2 pasos y parpadeaba la pose de salto en mitad del deslizamiento).

## Alternativas descartadas
- **Mantener "bajar andando desliza":** es lo que se probó y no era lo que el autor quería.
- **Agacharse también en rampas:** en una rampa abajo es deslizar; se evita que una tecla haga dos cosas a la vez.
- **Poder saltar agachado:** se descartó para que agacharse sirva para pasos bajos (con el techo encima no se podría saltar de todos modos).

## Consecuencias
- **`level_check` no pulsa abajo:** no modela el agachado ni el deslizamiento. Es seguro en una dirección (un hitbox menor y más velocidad solo ayudarían, así que una ruta que encuentra es real), pero **un nivel que EXIJA agacharse se daría por inalcanzable** hasta que la búsqueda aprenda a agacharse. Está en el backlog. La clave de estado conserva los bits del deslizamiento (hoy siempre a 0).
- Cualquier cosa nueva que mida o colisione con el jugador debe usar `player.height`/`bounds()` y no suponer 16 px.
- Los tests de rampas se reescribieron para pulsar abajo; hay tests nuevos (`crouch_test`) de la altura, la velocidad, el salto, el paso bajo y la salida al aire.
