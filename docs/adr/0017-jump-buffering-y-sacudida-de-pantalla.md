# ADR-0017: Jump buffering y sacudida de pantalla

- Estado: Aceptada
- Fecha: 2026-11-23
- Completa: ADR-0004 (que descartó el jump buffering "por ahora")

## Contexto
El salto ya tenía altura variable y coyote time (ADR-0004). La revisión externa sugirió también el *jump buffering*, y con enemigos los golpes pedían más respuesta visual.

## Decisión
**Jump buffering** (`domain/physics/player_physics`):
- Una **pulsación nueva** del salto que no se puede usar todavía (en el aire, sin coyote time) se recuerda `kJumpBufferSteps = 6` pasos (0,1 s, la misma lógica que el coyote: se gasta después de comprobar el salto). Si el jugador toca suelo dentro de esa ventana, **salta en ese paso**.
- Estado nuevo `Player::jump_buffer_steps`. No hay doble salto (un salto gasta tanto el buffer como el coyote), mantener pulsado al aterrizar no salta (hace falta pulsación nueva en el momento de la pulsación) y **un golpe olvida la pulsación** (`applyDamage` y el estado herido lo ponen a 0). Agachado tampoco salta, pero el buffer se va gastando.
- **`level_check` lo ignora a propósito:** un salto recordado no alcanza nada nuevo (equivale a pulsar de nuevo en el paso del aterrizaje, que la búsqueda ya prueba), pero meter el contador en la clave de estado multiplicó el espacio de búsqueda y superó el límite de 20 M estados. Tras cada paso el comprobador pone el buffer a 0, y el número de estados queda igual que antes (~9 M).

**Sacudida de pantalla** (`presentation/screen_shake`): al recibir un golpe, el dibujo se desplaza unos píxeles enteros durante `kShakeStepsAfterHit = 14` pasos (≈ 0,23 s), con amplitud inicial 3 que se desvanece y un vaivén que alterna cada paso. Es **una función sin estado** (ADR-0008): se calcula con `invulnerable_steps`, que ya cuenta hacia atrás desde el golpe, sin evento ni estado propio. El renderer mueve el *viewport* (se tapa el borde descubierto con negro) y no sacude el menú de pausa ni se aplica pausado.

## Alternativas descartadas
- **Meter el buffer en la clave de `level_check`:** correcto en teoría, pero inviable por coste.
- **Evento `PlayerHurt` para disparar la sacudida:** habría que guardar un estado visual en el renderer; con `invulnerable_steps` basta.
- **Buffer más largo (≥ 10 pasos):** empieza a saltar "solo" cuando el jugador ya no lo esperaba; 6 pasos se sienten como ayuda, no como trampa.

## Consecuencias
- La altura del salto, su velocidad y el coyote time no cambian; los niveles siguen siendo alcanzables (`level_check` OK).
- No hay opción para desactivar la sacudida: encaja con la tarea de accesibilidad (reducir parpadeo y movimiento, backlog 25).
