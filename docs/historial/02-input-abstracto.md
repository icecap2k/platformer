---
title: "Input abstracto: el dominio habla de acciones, no de teclas"
epic: "E2 · Input abstracto"
date: 2026-11-17
summary: "Separamos las teclas de las acciones del juego con un puerto y un adaptador, para añadir mando sin tocar el dominio."
---

En muchos juegos pequeños el código de movimiento hace algo como `if (keys[SDL_SCANCODE_LEFT])`. Es rápido, pero ata la lógica del juego a SDL y al teclado. Si mañana quieres mando, tienes que tocar la física. Si quieres testear el movimiento, necesitas pulsar teclas de verdad.

## La idea: puertos y adaptadores

Es el mismo patrón que usarías en frontend con DDD (arquitectura hexagonal):

```
teclado ─┐
mando  ──┼──► adaptador ──► InputState ──► dominio
replay ──┘     (infra)       (puerto)
```

- **`InputState`** (dominio) es el *puerto*: describe lo que el jugador quiere hacer, con acciones abstractas.
- **`KeyboardInputMapper`** (infraestructura) es el *adaptador*: traduce teclas de SDL a ese `InputState`.

## El dominio: `InputState`

```cpp
struct InputState {
    bool left = false;
    bool right = false;
    bool jump = false;

    // -1 = moving left, 0 = still, +1 = moving right.
    int horizontalAxis() const {
        return (right ? 1 : 0) - (left ? 1 : 0);
    }
};
```

No hay ningún `#include` de SDL. Es un struct simple, y por eso lo puedes testear con total libertad:

```cpp
SUBCASE("both directions cancel out") {
    input.left = true;
    input.right = true;
    CHECK(input.horizontalAxis() == 0);
}
```

`horizontalAxis()` está en el dominio a propósito: la regla "izquierda y derecha a la vez se anulan" es una decisión del juego, no del teclado. Cuando añadamos mando, esa regla seguirá valiendo sin escribirla otra vez.

## La infraestructura: `KeyboardInputMapper`

Hace dos cosas, separadas:

```cpp
void KeyboardInputMapper::poll();           // pumps the SDL event queue
domain::InputState KeyboardInputMapper::state() const;  // reads held keys
```

El mapeo de teclas a acciones queda en un único sitio:

```cpp
input.left  = keys[SDL_SCANCODE_LEFT]  || keys[SDL_SCANCODE_A];
input.right = keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D];
input.jump  = keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W];
```

Dos decisiones que merece la pena explicar:

1. **Teclas "mantenidas", no eventos.** Para moverse nos interesa "¿está pulsada ahora?", así que usamos `SDL_GetKeyboardState` en vez de reaccionar a cada evento `KEY_DOWN`. Más adelante, para el salto, puede que necesitemos saber también "¿se acaba de pulsar?"; lo veremos en E3.
2. **Salir no es una acción del juego.** `ESC` y cerrar la ventana viven en `quitRequested()`, fuera de `InputState`. Salir de la aplicación es una preocupación de la aplicación, no del personaje; si lo metiéramos en `InputState`, el dominio tendría que saber que existe un "cerrar programa".

## Verlo funcionando

Todavía no hay personaje, así que en `main.cpp` dibujamos tres cajas (izquierda, salto, derecha) que se iluminan mientras pulsas las teclas. Es un indicador temporal que se retirará en E3.

```bash
cmake -S game -B build && cmake --build build
ctest --test-dir build --output-on-failure
./build/platformer
```

Prueba con flechas o `A`/`D`, y `Espacio`, `W` o flecha arriba. Si pulsas izquierda y derecha a la vez, ambas cajas se iluminan (cada una refleja su tecla), pero `horizontalAxis()` valdría 0.

## Qué dependencias tiene ahora cada pieza

```
domain          ← InputState, FixedTimestep (sin SDL)
infrastructure  ← Window, KeyboardInputMapper (SDL3 + domain)
platformer      ← main.cpp conecta mapper → InputState → simulación
tests           ← solo domain
```

## Siguiente: E3

Ahora sí aparece el protagonista: un `Player` con posición y velocidad, gravedad, movimiento horizontal y una caja de colisión (AABB). Toda esa física será dominio puro con tests, alimentada por el `InputState` que acabamos de crear.
