---
title: "Un salto más fino: altura variable, coyote time y volver a pulsar"
epic: "Extra · Jugabilidad"
date: 2026-11-17
summary: "Tres retoques al salto que cambian mucho la sensación de control, y por qué el comprobador de niveles tuvo que enterarse."
---

El salto funcionaba, pero se sentía rígido: siempre igual de alto, con la plataforma justa para salir y con rebote infinito si mantenías pulsado. Tres retoques lo arreglan, y los tres viven en el **dominio**, sin tocar SDL.

## 1. Altura variable

Si sueltas el botón mientras subes, el salto se acorta. Al despegar, la velocidad es `kJumpSpeed = 5`; al soltar, se limita la subida a `kJumpCutSpeed = 2`:

```cpp
if (!input.jump && player.velocity_y < -kJumpCutSpeed) {
    player.velocity_y = -kJumpCutSpeed;
}
```

Un toque da un saltito; mantenerlo da el salto completo (unos 60 px con la integración discreta; la fórmula continua `v²/2g` da 62,5), igual que antes.

## 2. Coyote time

Es la cortesía de dejarte saltar **unos pasos después de pisar el borde**. El jugador lo percibe como "salté a tiempo" aunque el juego, estrictamente, ya lo diera por caído. El nombre viene del coyote de los dibujos animados, que sigue en el aire un momento tras pasar el precipicio.

Se guarda un contador en `Player`:

```cpp
if (player.on_ground) player.coyote_steps = kCoyoteSteps;  // 6 pasos = 0,1 s
// ...se comprueba el salto, y si no se salta en el aire:
else if (player.coyote_steps > 0) --player.coyote_steps;
```

Y el salto pregunta por el contador en lugar de por `on_ground`. El contador se gasta **después** de comprobar el salto, así que la ventana es de exactamente 6 pasos en el aire (la primera versión gastaba uno antes y daba solo 5: lo cazó una revisión externa y ahora hay un test de frontera con un borde real). Al saltar se pone a 0, para que no se pueda encadenar un segundo salto en el aire.

## 3. Hay que volver a pulsar

Antes, mantener `Espacio` hacía rebotar al jugador sin parar. Ahora `Player` recuerda si el botón estaba pulsado en el paso anterior (`jump_held`) y solo salta con una **pulsación nueva**.

Es lo mismo que el *"flanco"* de una señal: lo que importa no es que el botón esté abajo, sino que **acabe de bajar**. Y se guarda en el estado del jugador, no en el adaptador de teclado: así el dominio sigue sin saber de dónde vienen las órdenes y el comportamiento se puede testear sin ventana.

## Lo que se rompió: el comprobador de niveles

`level_check` explora todos los estados posibles del jugador. Ahora el estado incluye `coyote_steps` y `jump_held`, y el espacio de búsqueda creció tanto que **llegó al límite de 20 millones de estados** sin terminar: una moneda salía como inalcanzable solo porque la búsqueda se cortaba antes.

La solución no fue subir el límite, sino **no distinguir estados que se comportan igual**:

- En el suelo, el contador de coyote siempre se rellena: su valor previo da igual.
- En el aire sin coyote, mantener el botón pulsado ya no cambia nada.

Con esa normalización la búsqueda termina (unos 11,5 millones de estados, ~37 s) y las cuatro salas y las tres monedas siguen siendo alcanzables. Es una lección general: **cuando añades estado al dominio, el análisis que lo explora paga el precio.**

## Tests

Casos nuevos en `player_physics_test`:

- Soltar pronto da un salto claramente más corto.
- Mantener pulsado solo despega una vez; soltar y pulsar de nuevo salta.
- Coyote time, con una plataforma de verdad: se puede saltar hasta el último paso de la ventana y no el siguiente.
- No hay doble salto aprovechando el coyote.

Los tests antiguos de altura máxima y de techos ahora **mantienen el botón** mientras suben: es el comportamiento que antes se daba por supuesto y que el nuevo salto hace explícito.

## Siguiente

Quedan los tiles nuevos (plataformas de un solo sentido, pinchos), los enemigos y el sonido.
