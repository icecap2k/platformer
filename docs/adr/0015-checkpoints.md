# ADR-0015: Checkpoints y reaparición

- Estado: Aceptada
- Fecha: 2026-11-23
- Matiza: ADR-0005 (reinicio total al llegar a 0 corazones)

## Contexto
El ADR-0005 eligió el reinicio total y avisó: si resultaba frustrante, subir los checkpoints. Con los enemigos (ADR-0014) hay más formas de perder corazones por sorpresa.

## Decisión
- **Checkpoint = objeto de nivel que se toca** (`Checkpoint{x, y, size}`, clase `checkpoint` en Tiled, símbolo `K` en texto, `Room::checkpoints()`). Tocar uno que no sea el activo lo hace el nuevo (`GameSession::lastCheckpoint()`, evento `CheckpointReached`); tocarlo otra vez no hace nada. Solo hay **uno activo**.
- **Al quedarte sin corazones** (`hurtPlayer`): si hay checkpoint activo, el jugador **reaparece ahí** (`respawnAtCheckpoint`): salud completa, sin invulnerabilidad, de pie sobre la base del checkpoint, evento `PlayerRespawned{room}`. Si no hay ninguno, todo sigue igual que en el ADR-0005: `restart()` y `GameRestarted`.
- **Qué se conserva:** monedas y frutas ya recogidas, enemigos derrotados y estalactitas caídas **de las demás salas**. **Qué se repone:** los enemigos y las estalactitas **de la sala donde reapareces** (`EnemyPatrols::resetRoom`, `StalactiteTraps::resetRoom`), para no reaparecer al lado de un escarabajo que ya iba hacia ti o bajo una trampa a medias.
- **Sonido:** `PlayerRespawned` reutiliza el sonido de `Restart`; `CheckpointReached` tiene uno nuevo (`checkpoint`, dos notas ascendentes).
- **Dibujo:** `sprites/checkpoints.png` (`make_checkpoints.py`): linterna de cristal apagada y, la activa, encendida en turquesa con un halo que pulsa (dos fotogramas). Solo la activa brilla.
- **`level_check`:** un checkpoint inalcanzable falla el nivel, como una moneda. Colocados: `room_02` (justo pasados los pinchos) y `room_03`.

## Alternativas descartadas
- **Reaparecer en la entrada de la sala actual:** más suave pero sin nada que colocar; el autor prefirió checkpoints.
- **Activación automática al entrar en cada sala:** sin objetos, pero quita al diseñador el control de dónde se guarda.
- **Vidas limitadas / Game Over tras N muertes:** de momento no hace falta; con checkpoints el Game Over tiene poco sentido.

## Consecuencias
- El progreso (monedas, frutas) deja de perderse con cada muerte tras un checkpoint; el reinicio total solo ocurre sin ninguno.
- El checkpoint activo **no se guarda en disco**: guardado de partida sigue en el backlog (tarea 19) y deberá usar el `id` del objeto de Tiled, no el índice.
- Un nivel con un checkpoint dentro de la zona de un enemigo o de unos pinchos reaparece en peligro: lo vigila el diseñador (hay invulnerabilidad nula al reaparecer; si molesta, se le puede dar un momento).
