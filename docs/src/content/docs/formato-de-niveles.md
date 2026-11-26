---
title: Formato de los niveles
section: niveles
order: 1
summary: Las salas se editan en Tiled; qué puede contener cada una.
---

Las salas son mapas de [Tiled](https://www.mapeditor.org/), `game/assets/tiled/room_NN.tmj`. **Ese fichero es
el original**: se edita en Tiled y es lo que carga el juego.

## Editar una sala

1. Abre `game/assets/tiled/platformer.tiled-project` (trae las clases de objeto) y después la sala.
2. Los bloques y rampas se pintan en la capa `ground`.
3. Los objetos se crean en la capa `objects`: **Insertar rectángulo** (`R`), poner la **clase** y el tamaño.
4. Activa *Ver → Ajuste → Ajustar a la cuadrícula*.
5. Guarda, y desde la raíz del proyecto:

```bash
cmake --build build                              # copia los mapas junto al juego
./build/level_check game/assets/tiled room_01    # ¿se puede completar?
./build/platformer
```

## Qué contiene una sala

| Elemento | Texto | Clase en Tiled | Tamaño |
|---|---|---|---|
| suelo, bloques | `#` `B` `3` `4` | capa de tiles 1-4 | 1 tile |
| rampas | `/` `\` | capa de tiles 5 y 6 | 1 tile |
| moneda | `C` | `coin` | 8×8 |
| fruta | `F` | `fruit` | 8×8 |
| orbe | `R` `G` `Y` | `orb` + propiedad `colour` | 16×16 |
| checkpoint | `K` | `checkpoint` | 8×8 |
| salida | `X` | `exit` | 8×8 |
| escarabajo | `E` | `walker` | 16×16 |
| pinchos | `^` | `spikes` | 8×12 |
| estalactita trampa | `V` | `falling_stalactite` | 8×8 |

Las salas vecinas son propiedades del mapa: `north`, `south`, `east`, `west`.

Dos objetos necesitan cuidado:

- **Escarabajos:** su base debe estar **exactamente** sobre el borde superior de un bloque. Si flota o se hunde un
  píxel, se queda dando la vuelta sin moverse (una prueba lo detecta).
- **Orbes:** necesitan la propiedad `colour` (`red`, `green` o `yellow`). `red` es el valor por defecto de la
  clase y Tiled puede no guardarlo: si el juego no arranca, añádela a mano en ese objeto.

## Bocetos en texto

Para empezar una sala nueva rápido se puede dibujar como texto (cabecera, `---` y una letra por casilla) y
convertirla **una vez** a Tiled:

<!-- src: game/levels/example.txt -->
```text
................................C.......
............................BBBBBB......
```

```bash
./build/level_from_text boceto.txt game/assets/tiled/room_10.tmj
```

Los símbolos son los de la columna «Texto». Los elementos de dos casillas (`E` y orbes) se escriben en una: quedan
centrados en ella y apoyados en su borde inferior. El texto solo puede expresar objetos ajustados a la
cuadrícula.

## Una sala nueva

Crea `room_10.tmj`, enlázala con las propiedades `north`, `south`, `east` o `west` (en las dos salas) y añade
`"room_10"` a la lista de `loadWorld` en `main.cpp`.

## Consejos de diseño

- El jugador salta unos **5 tiles** de alto (menos, si el salto es corto).
- Una conexión norte o sur necesita un hueco en la **misma columna** en la otra sala.
- Después de cualquier cambio, pasa el [comprobador](../comprobador/).
