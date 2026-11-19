# ADR-0005: Salud, daño con retroceso y reinicio total

- Estado: Aceptada
- Fecha: 2026-11-19

## Contexto
Nada podía dañar al jugador. Se quiere una base reutilizable para pinchos, trampas, y más adelante enemigos y frutas que curan.

## Decisión
- **3 vidas = 3 corazones de salud** (`kMaxHealth`), un único modelo: las frutas futuras recuperan corazones.
- **Un solo camino para el daño:** `domain::applyDamage(player, amount, direction)` (`domain/rules/damage.{h,cpp}`). Quita corazones (nunca por debajo de 0), activa invulnerabilidad (`kInvulnerableSteps = 90`, 1,5 s), el retroceso (`kHurtSteps = 14`) y un mini salto, y anula el coyote.
- **Retroceso:** durante `hurt_steps` el jugador **ignora el input** y se mueve a `kKnockbackSpeed = 0.6` px/paso (≈ **8 px** en total); no se aplica el recorte del salto. El mini salto `kHurtHopSpeed = 1.7` ≈ **7 px**. Valores tras ajuste del autor: el primer ajuste lanzaba 17 px y saltaba 14 px, y se redujo a la mitad.
- **Dirección:** la aleja del peligro (centro del jugador frente al centro del peligro); si están alineados, contra su movimiento. Mientras está herido, el sprite **mantiene la mirada** (si no, se giraba de espaldas).
- **Al llegar a 0 corazones se reinicia todo** (`GameSession::restart()`): sala inicial, spawn, 3 corazones, 0 monedas y trampas colgando de nuevo. Evento `GameRestarted`.
- Presentación: pose `Hurt`, parpadeo mientras es invulnerable (3 pasos visible / 3 oculto) y HUD de corazones arriba a la derecha.

## Alternativas descartadas
- Reaparecer en la sala actual con la salud llena (más suave) y pantalla de Game Over inmediata (adelanta la épica de menús): el autor eligió el reinicio total, más duro y clásico.

## Consecuencias
- Si el reinicio total frustra, subir los **checkpoints** del backlog.
- Eventos `PlayerHurt{room, health_left}` y `GameRestarted` listos para el sonido.
- Los enemigos reutilizarán `applyDamage` sin cambiar el modelo.
