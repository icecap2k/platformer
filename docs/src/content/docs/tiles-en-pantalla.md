---
title: Tiles en pantalla
section: presentacion
order: 3
summary: Cómo una rejilla de números se convierte en roca con bordes y puntas (autotile).
---

Si cada casilla se dibujara igual, el nivel parecería un damero. La técnica se llama **autotiling**: el dibujo
de cada casilla se elige **según sus vecinas**.

## La máscara de lados

Para cada bloque se pregunta qué lados dan al aire. Cuatro preguntas de sí o no forman un número de 0 a 15, un
bit por lado:

<!-- src: game/src/presentation/autotile.cpp -->
```cpp
int exposureMask(bool north, bool south, bool east, bool west) {
    return (north ? kExposedNorth : 0) | (south ? kExposedSouth : 0) | (east ? kExposedEast : 0) |
           (west ? kExposedWest : 0);
}
```

| Lado al aire | Valor |
|---|---|
| arriba | 1 |
| abajo | 2 |
| derecha | 4 |
| izquierda | 8 |

El `|` suma bits. Un tile con aire arriba y a la izquierda es 1 + 8 = **9**.

## La hoja de rocas

La máscara es la **columna**. La **fila** es el material (suelo o roca) y una de 4 variantes de textura. Las dos
últimas filas son las rampas.

![Hoja de rocas](/img/sheets/rocks.png)

La variante sale de un **hash** de la posición: parece aleatoria, pero una sala se ve siempre igual y no hay que
guardar nada.

<!-- src: game/src/presentation/autotile.cpp -->
```cpp
    const int variant = static_cast<int>(h % kRockVariants);
    const int material = (tile_id == 1) ? 0 : 1;
    return material * kRockVariants + variant;
```

## Los bordes de la pantalla

Una casilla fuera del mapa cuenta como **rellena**, para no dibujar un contorno donde el mundo continúa en la
sala vecina.

<!-- src: game/src/presentation/game_renderer.cpp -->
```cpp
    const bool outside = column < 0 || row < 0 || column >= map.width() || row >= map.height();
    return outside || map.blocksFace(column, row, face);
```

El contorno y las puntas que se estrechan forman parte del **dibujo**; la colisión sigue siendo el tile
completo.
