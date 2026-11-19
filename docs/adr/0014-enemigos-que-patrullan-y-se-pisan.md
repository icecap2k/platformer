# ADR-0014: Enemigos que patrullan y se pisan

- Estado: Aceptada
- Fecha: 2026-11-19

## Contexto
Hay salud, daño (ADR-0005), peligros fijos (ADR-0006) y sonido por eventos (ADR-0013), pero nada se mueve por su cuenta. El backlog pedía enemigos básicos que reutilizaran `applyDamage` y `PlayerHurt`, y dejaba por decidir si se pueden pisar.

## Decisión
**Comportamiento** (decidido por el autor):
- **Se pisan.** Si el jugador toca al enemigo **cayendo** (`velocity_y > 0`) y con los pies a menos de `kStompDepth = 6` px de su parte alta, el enemigo muere y el jugador rebota (`kStompBounceSpeed = 3.2`). Mantener salto da el rebote completo; sin mantenerlo, el recorte de salto habitual lo deja en un saltito. Cualquier otro contacto (de lado, desde abajo, a su altura) es un golpe normal: `applyDamage`, un corazón, retroceso. Pisar funciona **también durante la invulnerabilidad**.
- **El primer enemigo (`walker`, un escarabajo de cueva) camina y da la vuelta**: avanza `kEnemySpeed = 0.35` px/paso (la del jugador es 1,5) y se gira ante una pared, una rampa o un borde sin suelo bajo el pie delantero (incluido el límite de la sala). Siempre está pegado al suelo: sin gravedad ni rampas por ahora.
- Muerto, **no vuelve** hasta el reinicio total (ADR-0005), como la estalactita.

**Datos y dominio**, el patrón de siempre (dato de nivel + estado de partida):
- `Enemy{AABB bounds}` (`domain/enemies/enemy.h`) es dato de nivel: **el rectángulo es dónde empieza y su hitbox**. `Room::enemies()` es una lista aparte.
- `EnemyState{x_offset, direction, alive}` y `EnemyPatrols` (por sala+índice) guardan lo que hace; `stepEnemy` es una función pura sobre el `TileMap`. `isStomp` y `bounceOffEnemy` (`enemy_contact.h`) deciden y aplican el pisotón.
- `GameSession::updateEnemies` mueve **solo los enemigos de la sala actual** (los de otras salas se quedan quietos mientras no estás) y resuelve el contacto, entre los peligros fijos y las estalactitas. Evento nuevo `EnemyStomped{room, index}`; el daño sigue emitiendo `PlayerHurt`. `soundFor` lo traduce al sonido nuevo `Stomp` (un "boing" aplastado, en `make_sounds.py`).
- Carpeta nueva `domain/enemies/`; el resto de `domain/` no se reorganiza todavía (tarea 23 del backlog).

**Niveles y herramientas:** objetos de Tiled de clase `walker` (añadida a `platformer.tiled-project`), símbolo `E` en el formato de texto (un tile entero). Colocados uno en `room_03` y otro en `room_04`.
- **`level_check` no los modela** (como las estalactitas, ADR-0003): la búsqueda ya tarda ~45 s y un enemigo que se mueve multiplica los estados. El informe imprime cuántos hay y avisa de que no se tienen en cuenta.

**Dibujo:** `sprites/enemies.png`, dos fotogramas de 8×8 (patas en dos fases) que genera `make_enemies.py`; el escarabajo mira a la derecha y el renderer lo espeja al ir a la izquierda. Violeta frío para distinguirlo de las monedas y las frutas, que son cálidas.

## Alternativas descartadas
- **Solo esquivarlos** (peligro móvil sin forma de eliminarlo): más simple, pero no hay respuesta activa.
- **Que caiga por los bordes:** exigiría gravedad y colisión vertical del enemigo; el "da la vuelta" basta.
- **Modelarlos en `level_check`:** correcto pero demasiado caro hoy; se revisará con la poda de la tarea 13.

## Consecuencias
- Un nivel podría ser "ganable" según `level_check` y exigir pisar o esquivar un enemigo: igual que con las estalactitas, es responsabilidad del diseñador de niveles.
- Pendiente: enemigos sobre rampas y con gravedad, otros tipos (voladores, que persiguen), animación de aplastamiento al morir y afinar de oído el sonido `stomp`.
