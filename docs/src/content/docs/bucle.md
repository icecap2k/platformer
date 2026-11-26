---
title: El bucle del juego
section: arquitectura
order: 2
summary: Qué ocurre en cada fotograma y por qué la simulación va a pasos fijos.
---

Un juego es un bucle que no para: **leer la entrada → avanzar el mundo → dibujar**.

## Paso fijo

El mundo avanza en **pasos de 1/60 de segundo**, siempre iguales. Si cada fotograma avanzara "lo que haya
tardado" (*delta time*), un salto mediría distinto según el ordenador, y el comprobador de niveles no podría
demostrar nada. Con paso fijo, la misma entrada produce **siempre** el mismo resultado.

El tiempo real, que sí es irregular, se acumula y se convierte en un número entero de pasos:

<!-- src: game/src/domain/time/fixed_timestep.cpp -->
```cpp
    int steps = 0;
    while (accumulator_ns_ >= step_ns_ && steps < kMaxStepsPerAdvance) {
        accumulator_ns_ -= step_ns_;
        ++steps;
    }
```

El límite (`kMaxStepsPerAdvance`) evita la "espiral de la muerte": si el ordenador se atasca, no intenta
recuperar todo lo perdido y atascarse aún más.

Consecuencia práctica: las velocidades del juego se miden en **píxeles por paso**, no por segundo.

## El bucle completo

Está en `main.cpp`. Primero la entrada y los menús; después, tantos pasos como toquen (solo si se está
jugando); y por último un dibujo:

<!-- src: game/src/main.cpp -->
```cpp
            const int game_steps = flow.screen() == application::Screen::Playing && session ? steps : 0;
            for (int i = 0; i < game_steps; ++i) {
                for (const application::GameEvent& event : session->update(input)) {
                    report(event);
                    if (std::holds_alternative<application::GameWon>(event)) {
                        flow.win();
                    }
                    if (const auto sound = presentation::soundFor(event)) {
                        audio.play(presentation::soundName(*sound));
                    }
                }
            }

            renderer.draw(session ? &*session : nullptr, flow);
```

Fíjate en el reparto:

1. `session->update(input)` aplica las reglas y **devuelve lo que ha pasado** (eventos).
2. `main` reacciona: lo escribe en la consola, cambia de pantalla si se ha ganado y reproduce un sonido.
3. `renderer.draw` **solo lee** el estado y lo pinta. Nunca cambia las reglas.

El dibujado va sincronizado con la pantalla (*vsync*), que evita tirones; los pasos de simulación siguen siendo
60 por segundo aunque tu monitor vaya a 120 Hz.

Sigue con [datos y estado](../dominio-vision/).
