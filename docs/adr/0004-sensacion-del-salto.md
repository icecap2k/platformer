# ADR-0004: Sensación del salto

- Estado: Aceptada
- Fecha: 2026-11-17 a 2026-11-19

## Contexto
El salto inicial era siempre igual de alto, no perdonaba y rebotaba si se mantenía pulsado.

## Decisión
En el dominio (`domain/physics/player_physics.{h,cpp}`, valores en pasos de 1/60 s y px/paso):

- **Altura variable:** soltar el botón mientras se sube limita la velocidad de subida a `kJumpCutSpeed = 1.7`. Un toque da un saltito; mantenerlo da el salto completo.
- **Coyote time:** `kCoyoteSteps = 4` pasos **en el aire** tras dejar un borde (≈ 0,07 s). El contador se gasta *después* de comprobar el salto, para que la ventana sea exactamente ese número (la primera versión daba uno menos). Saltar lo pone a 0: no hay doble salto.
- **Pulsación nueva:** hace falta soltar y volver a pulsar (`jump_held` en `Player`); mantener no rebota. Vive en el dominio, no en el teclado, para que el dominio siga sin saber de dónde vienen las órdenes.
- **Altura del salto:** `kJumpSpeed = 4.2` ≈ **44 px** (`v²/2g`). Se redujo desde 5,0 (≈ 62 px) a pedido del autor, que lo veía muy alto; "al 70 %" se interpretó como **altura**, y como crece con el cuadrado de la velocidad se escaló con √0,7. `kJumpCutSpeed` se escaló igual (de 2,0 a 1,7).
- El coyote time empezó en 6 pasos y se bajó a 4 al probarlo.

## Alternativas descartadas
- *Jump buffering* (recordar un salto pulsado justo antes de aterrizar): lo sugirió la revisión externa; se descartó entonces y se añadió después, ver ADR-0017.

## Consecuencias
- Estado nuevo en `Player` (`coyote_steps`, `jump_held`) que el comprobador de niveles debe incluir en su clave (ADR-0003).
- Los tests de altura usan las constantes (`kJumpSpeed²/2g`), no cifras fijas; los de techos y plataformas mantienen el botón pulsado mientras suben.
- Cualquier cambio de estas constantes exige repasar `level_check`.
