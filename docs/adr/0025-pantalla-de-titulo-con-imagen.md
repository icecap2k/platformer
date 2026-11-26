# ADR-0025: Pantalla de título con un dibujo del autor

- Estado: Aceptada
- Fecha: 2026-11-26

## Contexto
El autor aportó un dibujo (la protagonista ante la boca de una cueva) y pidió usarlo en la pantalla inicial. Es una excepción a ADR-0007 (todo el arte sale de scripts) y cambia el aspecto del título de ADR-0016.

## Decisión
- El original se guarda en `game/art_sources/title.jpg`; `make_title_screen.py` recorta la franja 4:3 que conserva a la protagonista y la cueva y la reduce a **960×720** (`background/title.png`).
- Se dibuja ocupando la pantalla lógica de 320×240: como la ventana es ×3, se ve con su detalle.
- El nombre va sobre el cielo; el menú, en un panel semitransparente a la izquierda.

## Alternativas descartadas
- **Reducirlo a 320×240:** pierde casi todo el detalle.
- **Redibujarlo con un script:** no se parecería al original.

## Consecuencias
- El script solo funciona en macOS (`sips`) porque la biblioteca estándar de Python no lee JPG; el PNG está en el repositorio, así que compilar no lo necesita.
- El estilo del título es más detallado que el del juego.
