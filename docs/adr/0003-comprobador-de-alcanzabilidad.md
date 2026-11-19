# ADR-0003: Comprobador de niveles que reutiliza la física real

- Estado: Aceptada
- Fecha: 2026-11-17 (actualizada el 2026-11-19 con peligros)

## Contexto
Dibujar a ojo engaña: una plataforma puede estar 8 px demasiado alta. Se quería **demostrar** que cada sala y cada moneda se pueden alcanzar, no estimarlo.

## Decisión
`level_check` (en `game/tools/`, enlaza la misma librería `domain` que el juego) hace una **búsqueda en anchura** sobre el estado `(sala, jugador)` probando 6 combinaciones de entrada por paso (izq/der/quieto × salto sí/no), con `stepPlayer`, `confineToMap`, `exitedThrough` y `enterRoom` reales. También se ejecuta como test de CTest (`shipped_levels_reachable`).

- La clave de estado cuantiza posición (0,5 px) y velocidad (0,1) para que el espacio sea finito, e incluye el estado del salto, **normalizado a lo que aún puede cambiar el futuro** (en suelo el coyote siempre está lleno; en el aire sin coyote, `jump_held` no importa). Sin esa normalización se agotaba el límite de 20 M de estados.
- **Los pinchos son letales:** se descartan los estados en que el jugador toca uno; una ruta que los atraviesa no cuenta.
- **Las estalactitas trampa NO se modelan** (son dinámicas y se pueden esquivar). Limitación consciente: un nivel podría aprobarse y exigir esquivarlas.
- El jugador aparece en el aire (y=60) en la partida real; en los tests se coloca en el suelo para medir qué puede el salto.

## Alternativas descartadas
- Reescribirlo en Python con copias de las reglas: se desincroniza al cambiar la física.

## Consecuencias
- Cambiar la física (salto, gravedad, coyote) se valida contra los niveles reales: al bajar el salto al 70 % las 4 salas siguieron siendo ganables, pero las plataformas de `room_02` están a 40 px y el salto llega a ~42: **margen justo**.
- Coste creciente: ~40-65 s y ~11 M de estados; cada estado nuevo (salud, invulnerabilidad) lo agranda. Está en el backlog (rendimiento, etiqueta de test lento, CI).
