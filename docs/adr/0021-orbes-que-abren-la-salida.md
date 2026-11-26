# ADR-0021: Tres orbes que abren la salida

- Estado: Aceptada
- Fecha: 2026-11-26

## Contexto
El autor quería un coleccionable nuevo: tres esferas grandes (rojo-naranja, verde y amarilla) con burbujas, volumen y un latido, flotando. Decisiones del autor: las **tres repartidas entre las salas del nivel** (no tres por sala), **abren la salida**, y al cogerlas queda su **silueta a trazos**. Sustituye la parte de ADR-0018 que decía que ganar solo exige llegar a la salida.

## Decisión
- `Orb{x, y, size, colour}` es dato de nivel (clase `orb` de Tiled con la propiedad `colour`; símbolos `R`, `G`, `Y`, de dos tiles). Qué orbes se han cogido es estado de la partida (`PickupCollection`).
- **La salida solo funciona con todos** (`GameSession::exitOpen()`); un mundo sin orbes la tiene abierta. Se conservan al reaparecer en un checkpoint y se pierden en el reinicio total.
- Eventos `OrbCollected` (sonido `orb`) y `GateOpened` (sonido `gate`).
- **Dibujo** (`make_orbs.py`, `orbs.png`): celdas de 26×26, 12 fotogramas de latido con burbujas que suben, sombreado a partir de la normal de una esfera, y la silueta a trazos. Iconos propios de 11×11 para el marcador, arriba en el centro.
- **Reja:** la salida está cerrada con un rastrillo; al entrar en su sala con los tres orbes, **sube** (`exitSpriteFrame`, 12 fotogramas tras 0,35 s) y suena solo a hierro. Se repite en cada visita. La animación es del renderer; el aviso para el sonido, un evento.
- **Colocación:** salas 03 (roja, fuera del camino principal), 06 (verde) y 08 (amarilla).
- `level_check` exige poder tocar los tres.

## Alternativas descartadas
- **Tres orbes en cada sala** o **uno por sala**: el autor quiso tres en total.
- **Solo contador** o **un poder por color**: el autor eligió que abran la salida.
- **Sonido de la reja con golpes y retumbar:** probado; el autor prefirió solo el ruido de la reja.

## Consecuencias
- Hay que explorar, incluida la sala 03.
- `level_check` comprueba que cada orbe se alcanza desde el inicio, **no el orden**: una sala de la que no se pueda volver podría dejar la partida sin salida. Se ha comprobado a mano que desde la sala 09 se alcanza todo (`level_check game/assets/tiled room_09 20 190`); conviene repetirlo al cambiar salas.
- La salida se abre en el juego en cuanto se tiene el tercer orbe, aunque la animación aún no haya terminado.
