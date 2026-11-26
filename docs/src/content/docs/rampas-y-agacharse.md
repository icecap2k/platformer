---
title: Rampas y agacharse
section: dominio
order: 4
summary: Apoyarse en una pendiente, deslizarse y pasar por huecos bajos.
---

## Estar sobre una rampa

El jugador se apoya en una rampa **por el centro de sus pies**. Después de moverse, `resolveSlope` busca la
superficie bajo ese punto y lo coloca encima:

<!-- src: game/src/domain/physics/player_physics.cpp -->
```cpp
    if (player.velocity_y < 0.0F) {
        return;
    }
    const SlopeContact contact = findSlope(player, map, was_on_ground ? kSlopeSnap : 0.0F);
    if (contact.found) {
        player.y = contact.surface_y - player.height;
        player.velocity_y = 0.0F;
        player.on_ground = true;
    }
```

- Si está **subiendo** (saltando), la rampa no le atrapa.
- Si ya estaba en el suelo, se le "pega" hasta `kSlopeSnap` píxeles hacia abajo. Sin esto, bajar andando sería
  una sucesión de pequeñas caídas.

Hay dos detalles que cuestan descubrir: en la rampa, los pocos píxeles inferiores del cuerpo no chocan con las
paredes (el pie delantero se hunde en la pendiente), y al salir por abajo se le baja al suelo llano
(`snapToFloorBelow`).

## Deslizarse

Andar por una rampa es andar. **Pulsando abajo** se desliza:

<!-- src: game/src/domain/physics/player_physics.cpp -->
```cpp
    } else if (ramp.found && input.down) {
        player.slide_direction = ramp.downhill;
        player.slide_steps = kSlideCarrySteps;  // refilled while the key is held on the ramp
        player.velocity_x = static_cast<float>(ramp.downhill) * kSlideSpeed;
```

Al soltar, o al acabarse la rampa, conserva el impulso unos pasos que se van apagando. Se corta si empujas en
contra, chocas o te hieren.

## Agacharse

Abajo en suelo llano reduce la caja a la mitad **sin mover los pies**, y anda más despacio. Para levantarse
tiene que **haber hueco**:

<!-- src: game/src/domain/physics/player_physics.cpp -->
```cpp
    } else if (!wants_to_crouch && player.crouching && canStandUp(player, map)) {
        player.y -= kStandingHeight - player.height;
        player.height = kStandingHeight;
        player.crouching = false;
    }
```

Así un pasadizo bajo te mantiene agachado aunque sueltes la tecla.

Decisiones: [ADR-0010](../decisiones/0010-rampas-de-45-y-deslizamiento/) y
[ADR-0011](../decisiones/0011-agacharse-y-deslizar-con-la-flecha-abajo/).
