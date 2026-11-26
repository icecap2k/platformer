---
title: "Un protagonista nuevo y gráficos que se ven"
epic: "E17 · Cambio de gráficos"
date: 2026-11-26
summary: "Una capucha azul que ondea, escarabajos al doble de tamaño, un checkpoint y una salida más grandes. Y por qué casi nada de la física tuvo que cambiar."
---

Esta vez no tocaba una mecánica, sino **cómo se ve** el juego. Lo hice en una rama aparte (`experiment/bigger-hero`), porque un cambio de aspecto puede acabar en la papelera, y fuimos ajustando a base de lanzar el juego y mirar.

## El protagonista, al cuarto intento

La primera versión era grande y llevaba capa: no convenció. La segunda, delgada y estilizada: tampoco. La que funcionó partió de una imagen de referencia: una figura **pequeña y achaparrada**, con una capucha grande, botas gordas y una mochila. La cara queda en sombra, sin ojos ni boca, y el color pasó de verde a naranja y finalmente a **azul**, que destaca mucho más sobre la cueva.

La lección de dibujo: si la capucha se pinta como una pieza encima del cuerpo, parece un sombrero. Pintando capucha, punta y cuerpo como **una sola forma** y trazando el contorno al final, queda integrada.

## La punta que ondea

La hoja `hero.png` es una rejilla: las **columnas** son las poses de siempre (`PlayerPose`) y las **filas**, cuatro momentos de la onda de la punta de la capucha. Así la pose y la onda son independientes:

```cpp
const SDL_FRect source{pose * kPlayerSpriteWidth, hoodFlutter(pose, time_ms) * kPlayerSpriteHeight,
                       kPlayerSpriteWidth, kPlayerSpriteHeight};
```

`hoodFlutter` es otra función pura del reloj (como las luciérnagas): ondea despacio en reposo y deprisa al caer, y se puede probar sin abrir una ventana.

## Dibujo y hitbox no son lo mismo

El personaje se dibuja en una celda de 32×28, pero **choca con la misma caja de 8×16**. El renderer centra el dibujo en la caja y alinea los pies. Gracias a eso no hubo que tocar la física ni rehacer salas. Lo mismo con el checkpoint (12×20) y la salida (22×32): más grandes a la vista, con la misma zona que se toca.

## Lo que sí cambia el juego: escarabajos de 16×16

Los enemigos son la excepción: su hitbox creció con el dibujo, porque pisar algo que se ve el doble de grande y fallar sería injusto. Para eso:

- El símbolo `E` del formato de texto ahora ocupa **dos tiles**, apoyado en el suelo.
- Se actualizaron las salas 03 a 09.
- El dibujo tiene un ojo básico, patas largas y **cuatro fotogramas** de paso en vez de dos.

Las frutas, que probamos al doble de tamaño y devolvimos al suyo, ahora **flotan un tile** por encima del suelo, y se distinguen mejor. `level_check` confirma que se siguen alcanzando; lo que no puede decir es si los escarabajos grandes han hecho alguna sala demasiado difícil. Eso toca jugarlo.
