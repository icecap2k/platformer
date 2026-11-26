---
title: Jugador y física
section: dominio
order: 3
summary: Cómo se mueve, choca y salta el personaje, paso a paso.
---

## El jugador son datos

`Player` solo guarda cosas; las reglas están fuera, en `stepPlayer`:

<!-- src: game/src/domain/entities/player.h -->
```cpp
struct Player {
    float x = 0.0F;  // top-left corner of the collision box
    float y = 0.0F;
    float velocity_x = 0.0F;  // pixels per simulation step
    float velocity_y = 0.0F;
    float width = 8.0F;
    float height = kStandingHeight;
    bool on_ground = false;
```

Su **caja de colisión** mide 8×16 (8×8 agachado). El dibujo es mayor; son cosas distintas
([Animaciones](../animaciones/)).

## Un paso

`stepPlayer(player, input, map)` avanza exactamente un paso, en este orden:

1. Descuenta la invulnerabilidad.
2. Decide si se agacha.
3. Calcula la velocidad horizontal (o la impone un golpe o un deslizamiento) y **mueve en X**.
4. Gestiona el salto.
5. Aplica la gravedad y **mueve en Y**.
6. Ajusta los pies a las rampas.

## Chocar: primero un eje y luego el otro

Si movieras en diagonal de golpe no sabrías qué cara has golpeado. Moviendo cada eje por separado es trivial:

<!-- src: game/src/domain/physics/player_physics.cpp -->
```cpp
    if (player.velocity_y > 0.0F) {
        const int row = lastTileIndexBefore(player.y + player.height, tile_size);
        if (anySolidInRow(map, row, first_column, last_column)) {
            player.y = static_cast<float>(row * tile_size) - player.height;
            player.velocity_y = 0.0F;
            player.on_ground = true;
        }
```

"Si caigo y la fila de mis pies tiene algo sólido, me coloco justo encima y estoy en el suelo." Subiendo es lo
mismo con la cabeza, y en horizontal con los costados.

> Por eso ninguna velocidad puede pasar de un tile (8 px) por paso: el jugador **atravesaría** una plataforma
> fina sin llegar a solaparla.

## Gravedad

<!-- src: game/src/domain/physics/player_physics.cpp -->
```cpp
    player.velocity_y = std::min(player.velocity_y + kGravity, kMaxFallSpeed);
    moveVertically(player, map);
```

Cada paso la velocidad vertical aumenta un poco, hasta un máximo. Con `kJumpSpeed = 4.2` y `kGravity = 0.2` un
salto completo sube unos 44 px.

## Un salto que se siente bien

Tres trucos clásicos, que el jugador no nota pero echa de menos si faltan:

| Truco | Qué hace | Constante |
|---|---|---|
| **Coyote time** | puedes saltar unos instantes después de salir de un borde | `kCoyoteSteps` = 4 |
| **Jump buffering** | si pulsas justo antes de aterrizar, salta al tocar suelo | `kJumpBufferSteps` = 6 |
| **Altura variable** | soltar el botón corta la subida: un toque = saltito | `kJumpCutSpeed` = 1.7 |

<!-- src: game/src/domain/physics/player_physics.cpp -->
```cpp
    if (input.jump && !player.jump_held) {
        player.jump_buffer_steps = kJumpBufferSteps;
    }
    if (hurt) {
        player.jump_buffer_steps = 0;
    }
    if (!hurt && !player.crouching && player.jump_buffer_steps > 0 && player.coyote_steps > 0) {
        player.velocity_y = -kJumpSpeed;
```

`input.jump && !player.jump_held` significa **pulsación nueva**: mantener el botón no hace rebotar. Los dos
"contadores de pasos" son todo el truco: uno recuerda que había suelo y otro que pulsaste.

## Lo que la física no hace

No sabe nada de los bordes de la pantalla: salir de una sala es una regla aparte ([Salas](../salas/)).

Decisiones relacionadas: [ADR-0004](../decisiones/0004-sensacion-del-salto/) y
[ADR-0017](../decisiones/0017-jump-buffering-y-sacudida-de-pantalla/).
