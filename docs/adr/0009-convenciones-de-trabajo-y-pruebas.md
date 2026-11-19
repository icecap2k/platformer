# ADR-0009: Convenciones de trabajo, pruebas y git

- Estado: Aceptada
- Fecha: 2026-11-19

## Contexto
Reglas que el autor ha ido fijando y técnicas que han funcionado, para no redescubrirlas en cada sesión.

## Decisión
**Idioma:** código, comentarios, nombres de tests, mensajes de commit y README en **inglés**; blog (`docs/src/content/blog`), ADR y `docs/BACKLOG.md` en **español**. El chat con el autor, en español.

**Ritmo:** una épica cada vez (código + tests + un post de blog en español) y parar para revisar. Los tests deben estar **siempre en verde** y, cuando se pueda, se verifica visualmente.

**Pruebas:**
- Las librerías de prueba **nunca enlazan SDL**.
- `cmake -S game -B build && cmake --build build && ctest --test-dir build --output-on-failure`; `./build/tests` para los unitarios (rápidos); `shipped_levels_reachable` tarda ~40-65 s.
- Tras tocar física o niveles, ejecutar `./build/level_check game/assets/tiled room_01`.
- **Captura sin ventana** con el renderer real (programa desechable, no se commitea): `SDL_CreateSoftwareRenderer` sobre un `SDL_Surface`, simular N pasos de `GameSession`, dibujar con `GameRenderer`, `SDL_SaveBMP`, ejecutar con `SDL_VIDEODRIVER=dummy`, convertir con `sips -s format png` y mirar la imagen. SDL3 está en Homebrew (`/opt/homebrew`); no hay `pkg-config`.
- CMake copia `game/assets/` junto al ejecutable con un **objetivo propio** (`copy_assets`, en cada compilación), de modo que cambiar solo una imagen o un mapa llega al juego sin recompilar código.

**Git:**
- Se prefiere **un único commit** al subir varias cosas; los mensajes terminan con `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`.
- **Preguntar antes de cada push** (publica en su cuenta). Nunca pedir ni escribir credenciales: el login de GitHub está en el llavero de macOS y `git push` funciona desde la terminal.
- No hay CLI `gh` (instalarla está en el backlog): los pull requests se abren desde el enlace que imprime `git push`.
- Hay un **hook de revisión previo al push** (`.claude/settings.json`, agente sin contexto sobre `git push*`). Se dejó **fail-open** a propósito: si el agente falla o agota el tiempo, el push sigue.
- Remoto: `https://github.com/icecap2k/platformer.git`, rama de trabajo actual `feat/tiled-post-and-better-jump`.

## Consecuencias
- Este registro y `docs/BACKLOG.md` son el contexto que debe leerse al empezar una sesión nueva.
