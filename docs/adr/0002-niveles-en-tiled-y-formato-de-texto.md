# ADR-0002: Niveles en Tiled (`.tmj`) y formato de texto como ayuda

- Estado: Aceptada
- Fecha: 2026-11-17

## Contexto
Los niveles son salas (pantallas de 40×30 tiles de 8 px) conectadas por sus cuatro bordes. Hacía falta un formato editable con ratón y otro versionable en Git.

## Decisión
- **Tiled `.tmj` (JSON, datos de capa en CSV) es la fuente de verdad** de cada sala. Reglas que exige el cargador (`infrastructure/assets/tiled_map_loader.cpp`):
  - se lee la **primera capa de tiles**; los ids 1-4 (y cualquier otro distinto de 0, 5 y 6) son **bloques sólidos** (`TileMap::isSolid`, la colisión por cajas); id 1 = suelo (roca más oscura), los demás = roca clara; los ids **5 y 6 son rampas de 45°** (no sólidas, tienen superficie: `isGround`, ver ADR-0010);
  - propiedades del **mapa** `north/south/east/west` (string) = id de la sala vecina (nombre de fichero sin extensión); sin propiedad, ese borde es un muro; los enlaces deben ser recíprocos;
  - **objetos** con campo *Clase* (o `type` en Tiled antiguo): `coin`, `spikes`, `falling_stalactite` (rectángulos; ver ADR-0006).
- `game/assets/tiled/platformer.tiled-project` define esas tres clases para el editor (**nunca se abrió en Tiled**: está sin verificar).
- Un **formato de texto** (`game/levels/*.txt`, convertido con `level_from_text`/`level_to_text`) sirve para esbozar y para ver diffs legibles. Símbolos: `.` vacío, `#` suelo, `B` ladrillo, `3`/`4` tiles 3 y 4 del tileset, `C` moneda, `^` pinchos, `V` estalactita trampa, `/` y `\` rampas (ids 5 y 6); cabecera `north/south/east/west: id`.
- **Una sola fuente de verdad por sala:** se esboza en texto, se convierte y, desde ahí, se edita solo en Tiled.
- El tileset va **incrustado** en cada `.tmj`; `tiled/tiles.png` es la paleta de 4 tiles que ve Tiled (la genera `make_rock_tiles.py`; desde ADR-0010 tiene 6 tiles: 4 bloques y 2 rampas).

## Alternativas descartadas
- Solo texto: no se puede diseñar con ratón. Solo Tiled: los diffs de JSON no se leen.

## Consecuencias
- Los índices de moneda dependen del orden de los objetos en Tiled; **antes de añadir guardado de partida hay que usar el `id` del objeto de Tiled**.
- Los símbolos `3` y `4` existen para que los mapas con esos tiles convivan sin pérdida con el texto.
- `toText` rechaza lo que no puede expresar (pinchos no alineados a tile, estalactitas que no miden un tile, monedas en tiles sólidos).
- Tests que dependen de los mapas reales: `shipped_levels_reachable`, el de `room_02` en `game_session_test`, `tiled_map_loader_test` (listas de salas y recuentos). Al añadir una sala hay que actualizar esas listas.
