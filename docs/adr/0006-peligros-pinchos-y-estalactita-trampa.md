# ADR-0006: Peligros: pinchos y estalactita trampa

- Estado: Aceptada (el dibujo y el tamaño de los pinchos, sustituidos por ADR-0020)
- Fecha: 2026-11-19

## Contexto
Primeros peligros del juego. Debían colocarse desde Tiled como las monedas y no romper la regla de que `isSolid` es el único sitio que decide qué es sólido (los peligros no son sólidos: se atraviesan y dañan).

## Decisión
**Datos del nivel frente a estado de la partida**, el mismo patrón que `Coin`/`CoinCollection`:

- **Pinchos:** objetos de Tiled de clase `spikes`; **el rectángulo es la hitbox** (el nivel decide cuánto perdona). `Hazard{AABB}` en `Room::hazards()`. En texto, `^` = un tile con la parte alta (3/8) libre.
- **Estalactita trampa:** objeto `falling_stalactite` → `FallingStalactite{AABB}` (dato) y `StalactiteTraps` (estado por sala+índice). Máquina de estados: `Idle → Shaking → Falling → Gone`.
  - Se activa cuando el jugador está debajo (±12 px a los lados) **sin nada sólido entre medias** (una repisa protege).
  - Tiembla `kShakeSteps = 30` (0,5 s) de aviso y, una vez que tiembla, cae aunque el jugador se vaya.
  - Cae con aceleración, a menos de un tile por paso (sin atravesar), daña **solo** en `Falling`, y se rompe al tocar suelo o al jugador. Queda `Gone` hasta el reinicio total. Evento `StalactiteFell`. En texto, `V`.
- **Dibujo de los pinchos:** un diente triangular de acero claro por tile (6 px) que, al juntar varios, forma una sierra; se dibuja **1 px más abajo** que el rectángulo: su última fila oscura se funde con el contorno del suelo y parecen clavados. Sobresale ~2 px por encima de la zona que daña. La estalactita trampa es más pálida y agrietada que las de adorno.
- `level_check` trata los pinchos como letales y no modela las estalactitas (ADR-0003).

## Alternativas descartadas
- Pinchos como tile sólido o como id de tile: `isSolid` los haría sólidos y no permitiría hitboxes a medida.
- Estalactita que se reinicia sola o al salir de la sala: más complejo; se eligió "un solo uso" por partida (el reinicio total la restaura).

## Consecuencias
- Solo hay pinchos de suelo; techo/pared están en el backlog.
- `Room` tiene un constructor con parámetros opcionales al final (`coins`, `hazards`, `stalactites`).
- Hay un test que verifica que en los niveles reales ningún peligro está enterrado y que las estalactitas cuelgan de algo sólido.
