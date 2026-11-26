# ADR-0007: Arte generado por scripts, y estilo visual

- Estado: Aceptada (el diseño del jugador y `make_player_sprites.py`, sustituidos por ADR-0019; los adornos de 8×8, por ADR-0020)
- Fecha: 2026-11-17 a 2026-11-19

## Contexto
Hasta E7 el arte salió de scripts desechables que no estaban en el repositorio (los PNG eran la única fuente). Se quería poder iterar sobre el aspecto y no perderlo.

## Decisión
Los PNG de `game/assets/sprites` y `background` **son salidas**: la fuente son scripts de Python (solo biblioteca estándar, sin Pillow) en `game/tools/art/`, deterministas (semillas fijas) y con `--preview out.png` para ver una versión ampliada sin escribir la hoja:

| Script | Genera |
|---|---|
| `make_player_sprites.py` | `sprites.png`: 17 fotogramas del jugador (celdas de 12×18), monedas conservadas de la hoja existente, corazones del HUD (fila y=26) |
| `make_rock_tiles.py` | `rocks.png` (autotiles y rampas) y `tiled/tiles.png` (paleta de Tiled, 6 tiles) |
| `make_cave_background.py` | `background/cave.png` (320×240) |
| `make_cave_decor.py` | `decor.png`: setas, cristales, guijarros, raíces, estalactita, y fotogramas de la gota |
| `make_hazards.py` | `hazards.png`: pinchos y estalactita trampa |

**Estilo:** contorno oscuro grueso `(28,20,32)`; paleta de cueva violeta/azulada.
- **Jugador:** original, inspirado en la expresividad de *Celeste* (sin copiar sprites). Túnica verde oscura con **capucha que le tapa los ojos** (solo se ven nariz, boca y barbilla), cinturón, piernas desnudas finas y botas. La punta de la capucha sustituye al pelo y reacciona al movimiento. 17 fotogramas: 4 reposo, 6 carrera, subida, cima, caída, aterrizaje, golpe, deslizamiento y agachado.
- **Rocas (autotiles):** cada tile se elige según qué lados dan al aire (16 máscaras × 4 variantes × 2 materiales). Los extremos de plataforma acaban en **punta que se estrecha por debajo**, la cara superior queda plana e iluminada, la inferior irregular. **El contorno forma parte del dibujo**, no lo pinta el renderer. La colisión **no** se talla: sigue siendo un tile completo.
- Cada ejecución del script de arte puede regenerar la hoja entera; el de jugador conserva las monedas leyéndolas de la hoja en disco.

Los **sonidos** siguen la misma idea (ADR-0013): `game/tools/audio/make_sounds.py` sintetiza los WAV.

## Alternativas descartadas
- Pintar los PNG a mano y versionar solo el resultado: no se puede iterar ni reproducir.
- Instalar Pillow: se evitó una dependencia; se escribió un lector/escritor de PNG mínimo.

## Consecuencias
- Para cambiar un dibujo se edita el script y se vuelve a ejecutar; el README documenta los comandos.
- La numeración de fotogramas (`PlayerPose`, `kPlayerFrameCount`, columnas de `rocks.png`, constantes de `autotile.h` y `drip.h`) debe mantenerse **sincronizada con los scripts** a mano.
