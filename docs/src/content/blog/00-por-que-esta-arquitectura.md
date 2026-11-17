---
title: "Por qué esta arquitectura (y cómo se parece a DDD)"
epic: "E0 · Cimientos"
date: 2026-11-17
summary: "Separamos el dominio del juego de SDL para poder testearlo y crecer sin dolor."
---

En tus juegos anteriores (como Coffee Crisis) la lógica y SDL conviven: un `Sprite` sabe dibujarse y también moverse. Funciona, pero si quieres testear la física tienes que abrir una ventana. Aquí vamos a hacerlo como harías en frontend con DDD: **el dominio no sabe que existe SDL**.

## Las capas

```
game/src/
  domain/          C++ puro. Reglas, física, mundo. Sin SDL.
  application/     Orquesta casos de uso (GameSession).
  infrastructure/  Todo lo que toca SDL: ventana, teclado, cargar mapas.
  presentation/    Dibujar lo que dice el dominio.
  main.cpp         Raíz de composición: conecta las piezas.
```

La regla de dependencias es una sola: **las flechas apuntan hacia el dominio**. `infrastructure` puede incluir cabeceras de `domain`, nunca al revés.

| Frontend con DDD | Aquí |
|---|---|
| Entidades / value objects | `Player`, `Room`, `AABB` |
| Casos de uso / servicios de aplicación | `GameSession` |
| Puertos y adaptadores | `InputState` (puerto) y `KeyboardInputMapper` (adaptador) |
| Raíz de composición | `main.cpp` |

## Qué hicimos en E0

1. Creamos la estructura de carpetas y el repositorio git.
2. Un `CMakeLists.txt` con **tres piezas**: la librería `domain` (sin SDL), el ejecutable `platformer` (con SDL3) y el ejecutable `tests` (con doctest).
3. Un test de humo que demuestra que el dominio compila y se prueba sin tocar SDL:

```cpp
TEST_CASE("the domain compiles and links without SDL") {
    CHECK(domain::gameName() == "platformer");
}
```

La clave está en el CMake: `domain` **no enlaza** con SDL3, así que si alguien incluye `<SDL3/...>` desde el dominio, no compilará. La arquitectura queda vigilada por el compilador.

```cmake
add_library(domain STATIC src/domain/version.cpp)   # sin SDL
target_link_libraries(platformer PRIVATE domain SDL3::SDL3)
target_link_libraries(tests PRIVATE domain doctest::doctest)
```

## Cómo lo ejecutas

```bash
cmake -S game -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/platformer
```

## Siguiente: E1

Abriremos una ventana con SDL3 y montaremos el **bucle de juego con paso fijo a 60 Hz**, el mismo concepto que usabas en `demo5` con `DEMO_SPEED = 1000/60`, pero dejándolo bien separado.
