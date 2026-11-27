---
title: Gráficos y sprites
section: presentacion
order: 2
summary: Las hojas de sprites y por qué el arte se genera con programas.
---

## Qué es una hoja de sprites

Una sola imagen con muchos dibujos en una **rejilla**. Para pintar uno se calcula su rectángulo:
`x = columna * ancho`, `y = fila * alto`.

### La protagonista (`hero.png`)

Celdas de 32×28: **19 columnas** (poses) y **4 filas** (la melena ondeando).

![Hoja de la protagonista](/img/sheets/hero.png)

La **cabeza** está dibujada a mano como arte ASCII, a partir de una imagen de referencia: cada letra es un color.

<!-- src: game/tools/art/make_hero_sprites.py -->
```python
HEAD = [
    "............",
    "....HWHHH...",
    "...HrHHHHH..",
```

Brazos, piernas y el mechón que ondea se colocan **según la pose**, y al final un contorno negro rodea toda la
figura.

### Escarabajo (`enemies.png`)

Cuatro pasos y un fotograma aplastado.

![Hoja del escarabajo](/img/sheets/enemies.png)

### Murciélago (`bats.png`)

Arriba, tranquilo; abajo, atacando (ojos rojos). Cuatro fotogramas de aleteo y uno derrotado.

![Hoja del murciélago](/img/sheets/bats.png)

### Orbes (`orbs.png`)

Una fila por color: 12 fotogramas de latido y la silueta a trazos. Debajo, los iconos del marcador.

![Hoja de los orbes](/img/sheets/orbs.png)

### Salida (`exit.png`)

Abierta (2), cerrada (1) y la reja subiendo (12).

![Hoja de la salida](/img/sheets/exit.png)

### Objetos pequeños (`sprites.png`)

Monedas (fila y = 18), corazones (y = 26) y frutas (y = 34).

![Monedas, corazones y frutas](/img/sheets/sprites.png)

### Peligros y checkpoint

![Pinchos y estalactita trampa](/img/sheets/hazards.png)
![Checkpoint](/img/sheets/checkpoints.png)

## El arte es código

Las imágenes del juego no se pintan a mano (salvo el título, más abajo). Cada una la genera un script de Python de `game/tools/art/`, usando solo la
biblioteca estándar.

| Script | Genera |
|---|---|
| `make_hero_sprites.py` | `hero.png` |
| `make_sprites.py` | `sprites.png` |
| `make_enemies.py` | `enemies.png` |
| `make_bats.py` | `bats.png` |
| `make_orbs.py` | `orbs.png` |
| `make_exit.py` | `exit.png` |
| `make_checkpoints.py` | `checkpoints.png` |
| `make_hazards.py` | `hazards.png` |
| `make_rock_tiles.py` | `rocks.png` y la paleta de Tiled |
| `make_cave_decor.py` | `decor.png` |
| `make_cave_background.py` | `background/cave.png` |
| `make_glow_mushrooms.py` | `glow_mushrooms.png` (setas luminosas del fondo) |
| `make_fireflies.py` | `fireflies.png` |
| `make_title_screen.py` | `background/title.png`, a partir de un dibujo |

```bash
python3 game/tools/art/make_orbs.py                        # regenera la imagen
python3 game/tools/art/make_orbs.py --preview vista.png    # versión ampliada, sin tocar la hoja
cmake --build build                                        # copia los assets junto al juego
```

> **Regla:** se edita el script, nunca el PNG.

## La excepción: la pantalla de título

![Pantalla de título](/img/sheets/title.png)

Esta imagen no la genera un programa: es un dibujo del autor, guardado en `game/art_sources/title.jpg`. El script
solo **recorta** la parte 4:3 y la **redimensiona** a 960×720, tres veces la pantalla lógica, para que en la
ventana se vea con todo su detalle:

<!-- src: game/tools/art/make_title_screen.py -->
```python
LEFT = 250          # where the 4:3 slice starts, in pixels of the source
WIDTH, HEIGHT = 960, 720
```

## Dos formas de dibujar con código

**Como arte ASCII**, cada letra un color. Así están los pinchos:

<!-- src: game/tools/art/make_hazards.py -->
```python
SPIKE_ART = [
    "..O.....",
    ".OaO....",
    ".OaO....",
```

**Calculando cada píxel.** Los orbes se sombrean como una esfera real: para cada punto se obtiene hacia dónde
mira la superficie (su *normal*) y cuánta luz recibe:

<!-- src: game/tools/art/make_orbs.py -->
```python
            nx, ny = (x + 0.5 - CENTRE) / radius, (y + 0.5 - CENTRE) / radius
            nz = math.sqrt(max(0.0, 1 - nx * nx - ny * ny))
            light = max(0.0, nx * LIGHT[0] + ny * LIGHT[1] + nz * LIGHT[2])
```

## Cuidado: dos sitios que deben coincidir

El tamaño de las celdas y el orden de los fotogramas están en el script **y** en C++. Si cambias uno, cambia el
otro. Por ejemplo, `PlayerPose` define el orden de las columnas de `hero.png`.

Decisiones: [ADR-0007](../decisiones/0007-arte-generado-por-scripts/) y
[ADR-0019](../decisiones/0019-graficos-mas-grandes-y-nuevo-protagonista/).
