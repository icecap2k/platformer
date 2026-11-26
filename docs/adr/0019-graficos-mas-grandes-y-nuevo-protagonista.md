# ADR-0019: Nuevo protagonista y gráficos más grandes que su hitbox

- Estado: Aceptada (el diseño del protagonista, sustituido por ADR-0024)
- Fecha: 2026-11-26

## Contexto
El autor quería cambiar el personaje (más grande, con una capucha que se moviera, de otro color) y, ya con el cambio en marcha, que enemigos, frutas, checkpoint y salida se vieran mejor. Se probó en una rama aparte, jugando y corrigiendo a cada paso. Sustituye la parte del **jugador** de ADR-0007; el resto de ese ADR (arte generado por scripts) sigue igual.

## Decisión
**Protagonista** (`make_hero_sprites.py` → `sprites/hero.png`):
- Figura pequeña y compacta: una **capucha grande** que ocupa buena parte de su altura y se funde en una esclavina corta. Su borde delantero cae sobre una cara en sombra: **sin ojos, pelo ni boca**, solo nariz y barbilla. Brazos cortos, piernas cortas, botas grandes y una mochila naranja.
- **Azul** vivo, para destacar sobre la cueva violeta.
- La capucha, su punta y el cuerpo se pintan como **una sola forma** con un solo contorno, para que la capucha no parezca pegada.
- Celdas de **32×28**: una columna por `PlayerPose` (17) y una fila por momento de la **onda de la punta** (4). `hoodFlutter(pose, time_ms)` elige la fila: lenta en reposo, rápida al caer.
- La **hitbox sigue siendo 8×16**: el dibujo se centra en ella y se alinea por los pies. La física y los niveles no cambian.

**Otros gráficos:**
| Elemento | Dibujo | Hitbox |
|---|---|---|
| Escarabajo (`make_enemies.py`) | 16×16, 4 pasos y 1 chafado; cuerpo pequeño, ojo claro sin pupila, patas largas articuladas | **16×16** (antes 8×8) |
| Checkpoint (`make_checkpoints.py`) | 12×20, farol sobre un poste de piedra | 8×8, sin cambios |
| Salida (`make_exit.py`) | 22×32 | 8×8, sin cambios |
| Fruta y moneda | sin cambios de tamaño | las frutas **flotan un tile por encima del suelo** |

- El símbolo `E` del formato de texto pasa a ser un objeto de **dos tiles**: centrado en su tile y apoyado en su borde inferior. Los demás símbolos siguen ocupando uno.
- `make_player_sprites.py` pasa a llamarse `make_sprites.py` y solo dibuja `sprites.png` (monedas, corazones y fruta). Las filas de esa hoja no se mueven.

## Alternativas descartadas
- **Personaje grande y estilizado con capa**, al estilo de *Celeste* o *Shantae*: probado, no convenció.
- **Versión delgada, de brazos y piernas definidos:** tampoco; funcionó la figura achaparrada.
- **Amarillo-naranja:** se probó y el autor prefirió el azul.
- **Monedas y frutas al doble:** probadas y revertidas; los enemigos grandes se quedaron.
- **Agrandar la hitbox del jugador con el dibujo:** obligaría a revisar la física y las nueve salas.

## Consecuencias
- El jugador, el checkpoint y la salida **sobresalen de su hitbox**: lo que se ve no es exactamente lo que choca o se toca.
- Los escarabajos grandes sí cambian el juego: cuesta más saltarlos y necesitan más hueco. `level_check` sigue sin modelarlos (ADR-0014), así que la dificultad solo se comprueba jugando.
- `PlayerPose`, `kHoodFlutterFrames`, `kEnemyWalkFrames` y los tamaños de celda deben seguir **sincronizados a mano** entre scripts y renderer.
