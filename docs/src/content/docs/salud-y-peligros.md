---
title: Salud y peligros
section: dominio
order: 7
summary: Corazones, golpes, pinchos y estalactitas que caen.
---

El jugador tiene **3 corazones**. Todo lo que hace daño pasa por la misma función, así todos los golpes se
sienten igual.

## Un golpe

<!-- src: game/src/domain/rules/damage.cpp -->
```cpp
void applyDamage(Player& player, int amount, int direction) {
    player.health = std::max(0, player.health - amount);
    player.invulnerable_steps = kInvulnerableSteps;
    player.hurt_steps = kHurtSteps;
    player.hurt_direction = direction < 0 ? -1 : 1;
    player.velocity_y = -kHurtHopSpeed;  // a small hop, much lower than a jump
```

- Resta vida.
- **1,5 s de invulnerabilidad** (90 pasos), para no perder todos los corazones de golpe.
- Unos pasos sin control, empujado **lejos** de lo que le hirió, con un saltito.

`std::max(0, ...)` impide una vida negativa. `heal` hace lo contrario con `std::min`.

## Pinchos

Un `Hazard` es solo un rectángulo: **la zona que hace daño**. En las salas mide 12 px de alto y está en una
zanja, así que sobresale 4 px del suelo. El dibujo es más alto que la zona: rozar las puntas se perdona.

## Estalactitas trampa: una máquina de estados

```
Idle ──(pasas por debajo)──► Shaking ──(0,5 s)──► Falling ──(choca)──► Gone
```

<!-- src: game/src/domain/world/stalactite_traps.cpp -->
```cpp
    switch (state.phase) {
        case StalactitePhase::Idle:
            if (playerUnder(stalactite, map, player)) {
                state.phase = StalactitePhase::Shaking;
                state.shake_steps = kShakeSteps;
            }
            return false;
        case StalactitePhase::Shaking:
            if (--state.shake_steps <= 0) {
                state.phase = StalactitePhase::Falling;
                return true;
            }
            return false;
```

Cada fase solo puede pasar a la siguiente, y solo `Falling` hace daño. `playerUnder` comprueba además que **no
haya nada sólido en medio**: una repisa te protege.

## Perder el último corazón

- Con checkpoint: reapareces allí con la vida completa. Conservas monedas, frutas y orbes; los enemigos y
  trampas de esa sala se reponen.
- Sin checkpoint: la partida empieza de cero.

Decisiones: [ADR-0005](../decisiones/0005-salud-dano-y-reinicio/),
[ADR-0006](../decisiones/0006-peligros-pinchos-y-estalactita-trampa/) y
[ADR-0020](../decisiones/0020-pinchos-altos-y-decoracion-grande/).
