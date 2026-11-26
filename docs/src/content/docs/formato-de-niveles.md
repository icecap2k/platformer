---
title: Formato de los niveles
section: niveles
order: 1
summary: Las dos formas de dibujar una sala, Tiled y texto, y qué puede contener.
---

El juego carga mapas de **Tiled** (`game/assets/tiled/room_NN.tmj`). Para dibujar rápido existe además un
**formato de texto** que se convierte a Tiled.

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

## Un nivel en texto

Cabecera, `---` y una letra por casilla (40×30):

<!-- src: game/levels/example.txt -->
```text
................................C.......
............................BBBBBB......
```

Los elementos de **dos tiles** (`E` y orbes) se escriben en una casilla: quedan centrados en ella y apoyados en
su borde inferior. Los pinchos (`^`) van en una **zanja** de un tile en el suelo.

## Salas generadas

Las salas 05 a 09 se construyen con un script, que se lee casi como una descripción:

<!-- src: game/tools/make_rooms_05_09.py -->
```python
    r = Room()
    r.floor(gaps=[(13, 17)])
    r.fill(13, 17, 29, 29, "^")           # spikes at the bottom of the pit
    r.platform(12, 17, 22)               # a platform over the pit...
    r.put(14, 21, "C")                   # ...with a coin on it
```

Las salas 01 a 04 se editan a mano en Tiled (abre `platformer.tiled-project` para tener las clases).

## Consejos de diseño

- El jugador salta unos **5 tiles** de alto (menos, si el salto es corto).
- Una conexión norte o sur necesita un hueco en la **misma columna** en la otra sala.
- Después de cualquier cambio, pasa el [comprobador](../comprobador/).
