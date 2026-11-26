---
title: "Ventana SDL3 y bucle de juego con paso fijo"
epic: "E1 · Ventana y bucle"
date: 2026-11-17
summary: "Abrimos una ventana a 320x240 escalada y montamos un bucle que simula siempre a 60 pasos por segundo."
---

Un juego es un bucle infinito que repite tres cosas: **leer entrada, actualizar el mundo y dibujar**. La parte delicada es el *tiempo*: si actualizas el mundo con el tiempo real entre frames, la física cambia según la potencia del ordenador. La solución clásica es el **paso fijo** (*fixed timestep*): la simulación avanza siempre en trozos idénticos (aquí, 1/60 s) y lo que varía es cuántos trozos ejecutamos por frame.

## El problema: dibujar es variable, simular no

```
tiempo real:   |--16ms--|----40ms----|--16ms--|
frames:            1          2           3
pasos de 16ms:     1          2           1     <- siempre pasos iguales
```

Si un frame tarda 40 ms, ejecutamos 2 pasos de simulación para ponernos al día. El jugador siempre ve el mismo comportamiento, vaya el ordenador rápido o lento.

## La pieza clave vive en el dominio

`FixedTimestep` solo hace aritmética: recibe cuánto tiempo real ha pasado y devuelve cuántos pasos hay que simular. **No lee ningún reloj**, se lo dan de fuera, y por eso es testeable sin SDL:

```cpp
int FixedTimestep::advance(std::uint64_t elapsed_ns) {
    accumulator_ns_ += elapsed_ns;

    int steps = 0;
    while (accumulator_ns_ >= step_ns_ && steps < kMaxStepsPerAdvance) {
        accumulator_ns_ -= step_ns_;
        ++steps;
    }
    // ...
    return steps;
}
```

El tiempo sobrante se guarda en un **acumulador** para la próxima llamada. Además hay un tope de 5 pasos por llamada: si el juego se congela un segundo (por ejemplo, al arrastrar la ventana), no intentamos recuperar 60 pasos de golpe, porque eso tardaría más aún y se entraría en la llamada "espiral de la muerte".

Los tests lo demuestran sin abrir ninguna ventana:

```cpp
SUBCASE("leftover time is kept for the next call") {
    CHECK(timestep.advance(7) == 0);
    CHECK(timestep.advance(7) == 1);  // 14 accumulated -> 1 step, 4 left
    CHECK(timestep.advance(6) == 1);  // 4 + 6 = 10 -> 1 step
}
```

## La infraestructura: `Window`

Todo lo que toca SDL vive en `infrastructure/sdl/window.*`. Es una clase **RAII**: el constructor inicializa SDL y crea ventana y renderer; el destructor lo libera todo en orden inverso. Si algo falla, lanza una excepción en vez de devolver códigos de error, así el resto del código no tiene que comprobar nada.

Usamos una resolución **lógica de 320x240** que SDL escala por un factor entero (x3 = ventana de 960x720):

```cpp
SDL_SetRenderLogicalPresentation(renderer_, logical_width, logical_height,
                                 SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
```

Así dibujas siempre en coordenadas de 320x240 y los píxeles salen nítidos, igual que en tu `demo5`.

## Juntándolo en `main.cpp`

`main.cpp` es la **raíz de composición**: crea la ventana, crea el `FixedTimestep` y ejecuta el bucle:

```cpp
const int steps = timestep.advance(now_ns - previous_ns);
for (int i = 0; i < steps; ++i) {
    // simulation update goes here
}
render(window.renderer());
```

Fíjate en la separación: la **simulación** corre a pasos fijos; el **dibujado** corre una vez por frame. De momento el bucle de pasos está vacío; lo llenaremos con la física en E3.

## Cambios en la estructura del CMake

Ahora hay una librería nueva, `infrastructure`, que es la única que enlaza con SDL3:

```
domain          ← sin SDL
infrastructure  ← depende de domain + SDL3
platformer      ← depende de infrastructure
tests           ← solo depende de domain
```

## Cómo probarlo

```bash
cmake -S game -B build && cmake --build build
ctest --test-dir build --output-on-failure
./build/platformer      # cerrar con ESC o con la ventana
```

Verás una ventana azul oscuro con un borde blanco que marca los límites del lienzo lógico de 320x240.

## Siguiente: E2

Ahora mismo `main.cpp` lee la tecla ESC directamente de SDL. En E2 separaremos eso: el dominio hablará de **acciones** (`Left`, `Right`, `Jump`) y un adaptador traducirá teclas a acciones. Cuando añadamos mando, solo habrá que escribir otro adaptador.
