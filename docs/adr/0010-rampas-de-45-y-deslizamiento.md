# ADR-0010: Rampas de 45° y deslizamiento

- Estado: Aceptada, **salvo el disparador del deslizamiento, sustituido por el [ADR-0011](0011-agacharse-y-deslizar-con-la-flecha-abajo.md)** (se desliza al pulsar abajo, no al caminar)
- Fecha: 2026-11-19

## Contexto
La salida de `room_01` era una escalera de peldaños de 1 tile (tiles id 4) que obligaba a saltar cada escalón. El autor pidió **rampas de 45°** que la sustituyan, **solo de suelo**, y que **al bajar andando el personaje se deslice**. Hasta ahora la colisión era "un tile es sólido o no" (ADR-0002), así que es el cambio de física más delicado hecho hasta la fecha.

## Decisión
**Modelo de tiles** (`domain/world/tile_map.{h,cpp}`):
- Ids **5** (`kTileSlopeRight`, sube hacia la derecha ◢) y **6** (`kTileSlopeLeft`, sube hacia la izquierda ◣). `TileShape { Empty, Solid, SlopeRight, SlopeLeft }`.
- **`isSolid` significa ahora "bloque entero"** (ids 1-4 y cualquier otro distinto de 0, 5 y 6). Así la colisión por cajas y todo el código que ya lo usaba sigue igual.
- `isGround` = bloque o rampa ("tiene superficie"): lo usan las gotas, la estalactita trampa y la decoración. `slopeSurfaceY(col,row,x)` da la superficie de una rampa; `blocksFace(col,row,lado)` dice qué caras de la celda rellena (un bloque, todas; ◢, sur y este; ◣, sur y oeste), para calcular los contornos de los vecinos.
- **Regla de nivel:** una rampa se apoya siempre sobre sólido (relleno `B` debajo y en su lado alto). No hay rampas de techo.
- Texto: `/` = ◢ (id 5), `\` = ◣ (id 6). La paleta de Tiled pasa a 6 tiles.

**Física** (`domain/physics/player_physics.cpp`):
- El jugador se apoya en la rampa **por el centro de sus pies**: después de moverse, `resolveSlope` lo coloca en `surface − height` (sin atravesar: rampas de 8 px y caída máxima de 6 px/paso). Si ya estaba en el suelo, también sigue la superficie **hacia abajo** hasta `kSlopeSnap` = 8 px, para que bajar no sea una cadena de caídas. No actúa mientras sube (un salto abandona la rampa).
- Sobre una rampa, el borde inferior del hitbox (`width/2 + 0.5` px) **no cuenta para las paredes**: el pie delantero se hunde hasta 4 px en la diagonal y, si no, los bloques de relleno junto a la rampa lo frenarían.
- **Deslizamiento** (el disparador original, *al caminar hacia la bajada*, se cambió después a *pulsar abajo*; ver ADR-0011): en suelo sobre una rampa, con la tecla hacia la **bajada**, el jugador va a `kSlideSpeed = 2.4` px/paso (1,6× la carrera) y rellena `slide_steps = kSlideCarrySteps = 12`. **Quieto no se mueve; subir es una caminata normal.** Al acabar la rampa (o al saltar) conserva el impulso, que decae linealmente hasta 0 en 12 pasos; si se sigue pulsando en el mismo sentido, nunca baja de la velocidad de carrera. Pulsar al contrario, chocar con una pared o recibir un golpe lo cancela. Estado en `Player`: `slide_steps`, `slide_direction`.
- Presentación: pose `Slide` (fotograma 15; el jugador pasa a 16) mientras está en suelo con `slide_steps > 0`; tiene menos prioridad que `Hurt`.

**Arte:** dos filas nuevas en `rocks.png` (128×80): ◢ y ◣ con una máscara de 2 bits (sur expuesto, cara del lado alto expuesta). La hipotenusa lleva contorno y una línea clara debajo. Lo genera `make_rock_tiles.py`.

**Nivel:** `room_01` sustituye la escalera por una rampa de 5 tiles (sube 40 px) hacia una meseta en las columnas 32-36.

## Alternativas descartadas
- **Ángulos libres o curvas:** mucho más caro y no lo pidió el autor.
- **Deslizar también quieto sobre la rampa** (resbalar siempre) o "de aguante": el autor eligió que solo se deslice **al bajar andando**.
- **Rampas de techo:** fuera de alcance.
- **Muestrear el pie delantero en vez del centro:** evita el hundimiento de 4 px, pero complica los bordes de la rampa.
- **Tratar la rampa como sólida con colisión por píxel:** rompe la regla de que `isSolid` es la colisión de cajas.

## Consecuencias
- **Limitación conocida:** el pie delantero se hunde hasta 4 px en la rampa (se muestrea solo el centro).
- Los pinchos y los enemigos sobre una rampa no están soportados todavía (los pinchos tienen hitbox rectangular).
- `level_check` hereda las rampas sin cambios porque usa `stepPlayer`; su clave de estado incluye ahora el deslizamiento (ADR-0003). Subió a ~12 M de estados y ~61 s.
- Cualquier cambio de `kSlideSpeed`, `kSlideCarrySteps` o de la geometría exige repasar `level_check` y los tests de `slope_test`, que usan la geometría real de `room_01` (rampa, rellenos y meseta) y su espejo.
