---
title: Enemigos
section: dominio
order: 8
summary: El escarabajo que anda y cae, el murciélago que te busca, y cómo se les pisa.
---

Hay dos tipos. `Enemy` es el **dato del nivel** (dónde empieza, de qué tipo es); `EnemyState`, lo que está haciendo
en la partida.

| | Escarabajo (`walker`) | Murciélago (`bat`) |
|---|---|---|
| Se mueve | andando por el suelo | volando |
| Gravedad | sí | solo cuando cae derrotado |
| Te persigue | no | si te acercas |
| Se puede pisar | sí | sí |

## El escarabajo

### Caer y seguir el suelo

En cada paso busca **el suelo bajo el centro de su cuerpo**. Si no lo hay cerca, cae; si lo hay, se coloca encima.
Por eso sube y baja rampas, y uno colocado un poco alto o hundido se arregla solo.

<!-- src: game/src/domain/enemies/enemy_patrols.cpp -->
```cpp
    const auto under = groundNear(map, now.x + half, now.bottom(), kWalkerStepUp, kWalkerStepDown);
    if (!under) {
        fall(enemy, state, map);
        return;
    }
    state.y_offset += *under - now.bottom();  // feet on the ground (on a ramp it moves up or down)
```

### Dar la vuelta

Después mira **dónde estaría** y gira ante una pared, un borde, el límite de la sala o unos pinchos:

<!-- src: game/src/domain/enemies/enemy_patrols.cpp -->
```cpp
    if (blocked || ledge) {
        state.direction = -state.direction;
        return;
    }
    state.x_offset += static_cast<float>(state.direction) * kEnemySpeed;
```

Cerca de una rampa, igual que el jugador, la parte baja de su cuerpo no choca con los bloques de relleno.

## El murciélago

Tiene tres comportamientos:

```
patrulla ──(te acercas)──► ataque ──(te muerde)──► retirada ──► patrulla o ataque
    ▲                         │
    └────(te alejas)──────────┘
```

- **Patrulla:** dibuja un ocho alrededor de su sitio, con senos.
- **Ataque:** si estás a menos de su **radio de detección** (configurable en Tiled), vuela hacia ti. Lo deja si te
  alejas una vez y media ese radio.
- **Retirada:** tras morderte, se aparta un segundo.

### Bajo, para que puedas pisarlo

El murciélago apunta a la altura de **tus pies**, y solo actualiza esa altura cuando pisas suelo. Si saltas, no
sube detrás de ti: te queda debajo.

<!-- src: game/src/domain/enemies/enemy_patrols.cpp -->
```cpp
        if (player.on_ground) {
            state.chase_floor_y = player.y + player.height;
        }
        goal_x = player.x + player.width * 0.5F - now.width * 0.5F;
        goal_y = state.chase_floor_y - now.height;
        speed = kBatChaseSpeed;
```

Nunca atraviesa la roca: mueve un eje cada vez y descarta el que le metería en un bloque, así que se desliza por las
paredes. No sabe rodear obstáculos.

## Pisar o chocar

Para los dos, la pregunta es la misma:

<!-- src: game/src/domain/enemies/enemy_contact.cpp -->
```cpp
bool isStomp(const Player& player, const AABB& enemy) {
    return player.velocity_y > 0.0F && player.bounds().bottom() <= enemy.top() + kStompDepth;
}
```

**Cayendo** y con los pies cerca de su parte alta = pisotón: el enemigo queda derrotado 1 s y tú rebotas (más alto
si mantienes salto). El murciélago, además, cae al suelo. Cualquier otro contacto es un golpe para ti.

Decisiones: [ADR-0014](../decisiones/0014-enemigos-que-patrullan-y-se-pisan/) y
[ADR-0031](../decisiones/0031-murcielagos-y-escarabajos-con-gravedad/).
