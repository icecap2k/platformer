---
title: Enemigos
section: dominio
order: 8
summary: El escarabajo que patrulla, cuándo da la vuelta y cómo se le pisa.
---

Hay un tipo de enemigo: un escarabajo de 16×16 que camina por el suelo. `Enemy` es dónde empieza;
`EnemyState`, cuánto ha andado, hacia dónde va y si sigue vivo.

## Patrullar

En cada paso mira **dónde estaría** y decide: avanzar o darse la vuelta.

<!-- src: game/src/domain/enemies/enemy_patrols.cpp -->
```cpp
    // The foot that leads needs ground under it, or it would walk off.
    const int leading_column = state.direction > 0 ? last_column : first_column;
    const bool ledge = !map.isGround(leading_column, tileOf(now.bottom(), size));

    if (blocked || ledge) {
        state.direction = -state.direction;
        return;
    }
    state.x_offset += static_cast<float>(state.direction) * kEnemySpeed;
```

Gira ante una pared, una rampa, un borde sin suelo o **unos pinchos**.

## Pisar o chocar

Cuando el jugador toca un enemigo hay una sola pregunta:

<!-- src: game/src/domain/enemies/enemy_contact.cpp -->
```cpp
bool isStomp(const Player& player, const AABB& enemy) {
    return player.velocity_y > 0.0F && player.bounds().bottom() <= enemy.top() + kStompDepth;
}
```

**Cayendo** y con los pies cerca de su parte alta = pisotón: el enemigo queda aplastado 1 s y tú rebotas
(más alto si mantienes salto). Cualquier otro contacto es un golpe para ti.

El margen `kStompDepth` existe porque a 60 pasos por segundo una caída puede entrar varios píxeles antes de que
se detecte.

Decisiones: [ADR-0014](../decisiones/0014-enemigos-que-patrullan-y-se-pisan/) y
[ADR-0019](../decisiones/0019-graficos-mas-grandes-y-nuevo-protagonista/).
