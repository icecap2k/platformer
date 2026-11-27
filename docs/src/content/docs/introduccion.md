---
title: Introducción
section: empezar
order: 1
summary: Qué es el juego, cómo ponerlo en marcha y por dónde está cada cosa.
---

Es un **plataformas 2D** escrito en **C++20** con **SDL3**. Recorres una cueva de nueve pantallas conectadas,
recoges monedas y frutas, esquivas pinchos, estalactitas y escarabajos, y para escapar necesitas encontrar
**tres orbes** que abren la salida.

El proyecto tiene dos objetivos: que el juego funcione y que **sirva para aprender** a organizar un videojuego.

## Ponerlo en marcha

Necesitas CMake 3.20 o superior, un compilador de C++20 y SDL3 (en macOS, `brew install sdl3`). Desde la raíz:

```bash
cmake -S game -B build                              # prepara la compilación (solo la primera vez)
cmake --build build                                 # compila y copia los assets
ctest --test-dir build --output-on-failure          # pasa las pruebas
./build/platformer                                  # juega
```

| Acción | Teclado | Mando (nombres de Xbox) |
|---|---|---|
| Moverse | Flechas o `A` / `D` | cruceta o stick izquierdo |
| Saltar | `Espacio`, `↑` o `W` (mantener = salto alto) | `A` |
| Agacharse / deslizarse por una rampa | `↓` o `S` | cruceta o stick abajo |
| Pausa | `P` o `Esc` | `Start` |
| Silenciar | `M` | `Back` / `Select` |
| Volumen de la música | `-` y `+` | `LB` y `RB` |
| Menús | flechas y `Intro` | cruceta, `A` elegir, `B` volver |

## El mapa del repositorio

```
platformer/
├── game/
│   ├── src/
│   │   ├── domain/          las reglas del juego (C++ puro)
│   │   ├── application/     la partida y los menús
│   │   ├── infrastructure/  ventana, teclado, audio, lectura de mapas (SDL)
│   │   ├── presentation/    qué se dibuja y qué suena
│   │   └── main.cpp         conecta todo
│   ├── assets/              imágenes, sonidos y mapas
│   ├── levels/              un nivel de ejemplo dibujado como texto
│   ├── tools/               comprobador de niveles y generadores de arte y sonido
│   └── tests/               pruebas
└── docs/                    esta documentación
```

## Una ruta para aprender

1. [Las capas](../capas/): la idea que ordena todo el código.
2. [El bucle del juego](../bucle/): qué pasa 60 veces por segundo.
3. [Datos y estado](../dominio-vision/) y [el jugador](../jugador-y-fisica/): el corazón del juego.
4. [Cómo se dibuja](../renderizado/) y [las animaciones](../animaciones/).
5. [Recetas](../recetas/): cambia algo tú.
