---
title: Animaciones
section: presentacion
order: 4
summary: Elegir qué fotograma toca a partir del estado y del reloj.
---

Animar es cambiar de fotograma con el tiempo. Todo está en `presentation/animation`, en **funciones puras**:
reciben datos y devuelven un número, sin guardar nada. Por eso se prueban fácilmente.

## Un ciclo: tiempo ÷ duración

<!-- src: game/src/presentation/animation.cpp -->
```cpp
int coinFrame(std::uint64_t time_ms) {
    return static_cast<int>((time_ms / kCoinFrameMs) % 4);
}
```

`time_ms / kCoinFrameMs` cuenta cuántos fotogramas han pasado; `% 4` vuelve al primero tras el cuarto. Es la
base de casi todas.

## La pose del jugador

Aquí además importa **qué está haciendo**. Se pregunta por orden de prioridad:

<!-- src: game/src/presentation/animation.cpp -->
```cpp
    if (player.hurt_steps > 0) {
        return PlayerPose::Hurt;
    }
    if (player.on_ground && player.slide_steps > 0) {
        return PlayerPose::Slide;  // on the ground and still carrying the speed of a ramp
    }
```

Después: agachado, en el aire (subiendo, cima o cayendo según la velocidad vertical), aterrizando, quieto o
corriendo.

## Que la carrera se vea natural

La carrera son **8 fotogramas**: apoyo, hundimiento, paso y empuje, y los mismos cuatro con la otra pierna. En el
script se dibuja solo media zancada; la otra mitad sale **intercambiando los colores** de las dos piernas:

<!-- src: game/tools/art/make_hero_sprites.py -->
```python
SWAP = str.maketrans("PqBb", "qPbB")
RUN_LEGS = RUN_HALF + [[row.translate(SWAP) for row in frame] for frame in RUN_HALF]
```

Tres detalles dan naturalidad: las piernas **doblan la rodilla** en vez de girar como palos, el cuerpo **baja un
píxel** al apoyar, y cada brazo se mueve **al contrario** que la pierna de su lado, sin separarse del cuerpo.

## La melena al viento

La pose elige la **columna** y `hairFlutter` la **fila**: son independientes, y la onda va más rápida cuanto
más rápido te mueves.

<!-- src: game/src/presentation/game_renderer.cpp -->
```cpp
    const SDL_FRect source{static_cast<float>(static_cast<int>(pose)) * kPlayerSpriteWidth,
                           static_cast<float>(hairFlutter(pose, time_ms)) * kPlayerSpriteHeight,
                           kPlayerSpriteWidth, kPlayerSpriteHeight};
```

## Otros trucos

| Efecto | Cómo se hace |
|---|---|
| Mirar a un lado | un solo dibujo mirando a la derecha, **espejado** al ir a la izquierda |
| Parpadeo al ser herido | no dibujar al jugador 3 pasos de cada 6 |
| Flotar (frutas, orbes) | sumar a la `y` una tabla de desplazamientos |
| Reja que sube | fotograma según **cuánto lleva abierta** en pantalla |

<!-- src: game/src/presentation/animation.cpp -->
```cpp
    const std::uint64_t step = (ms_open - kGateDelayMs) / kGateFrameMs;
    if (step < static_cast<std::uint64_t>(kGateFrames)) {
        return kGateFirstFrame + static_cast<int>(step);
    }
    return exitFrame(time_ms);
```

La reja necesita recordar **desde cuándo** está en pantalla; eso lo guarda el renderer, y la función sigue
siendo pura.
