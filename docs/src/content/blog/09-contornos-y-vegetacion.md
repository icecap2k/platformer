---
title: "Un pase de arte: contornos gordos y vegetación"
epic: "Extra · Arte"
date: 2026-11-17
summary: "Contorno oscuro en tiles y personaje, y vegetación que sobresale del suelo. Cómo conseguir más estilo sin tocar la lógica del juego."
---

Con los sprites de E7 el juego ya tenía imágenes, pero aún se veía plano. Este pase de arte cambia **solo la capa de presentación y los PNG**: ni el dominio ni la física se tocan. Es la prueba de que separar capas funciona.

## Qué cambia

- **Contorno oscuro y grueso** alrededor del personaje y de los bloques.
- **Personaje más grande** (12×18 píxeles de dibujo) con gorra, cara, camiseta, manos y zapatos.
- **Tiles con más detalle**: tierra con piedrecitas, ladrillo con brillos y hierba con borde irregular.
- **Vegetación por fuera del suelo**: matas, flores, setas y arbustos encima, y enredaderas colgando por debajo.

## Contorno: se dibuja, no se pinta

Un contorno alrededor de cada bloque tiene un problema: si dibujas el borde dentro de cada tile, aparece **una rejilla** entre bloques contiguos. Solo queremos borde donde el bloque **toca el aire**.

En vez de dibujar 16 variantes de cada tile (una por cada combinación de vecinos), dibujamos el contorno **por código**, una línea de 1 píxel por cada lado expuesto:

```cpp
if (!solidOrOutside(map, column, row - 1)) {   // nada sólido encima
    const SDL_FRect edge{x, y, size, 1.0F};
    SDL_RenderFillRect(renderer_, &edge);
}
// ... lo mismo para abajo, izquierda y derecha
```

Las ventajas: los PNG de los tiles quedan **sin contorno** (más fáciles de dibujar y de reutilizar) y funciona con cualquier forma de terreno sin tener que preparar arte para cada caso.

Una decisión pequeña: una celda **fuera del mapa cuenta como sólida**. Así no se dibuja contorno en el borde de la pantalla, donde el mundo continúa en la sala siguiente. Si no, verías una línea oscura al pasar de una sala a otra.

## Un personaje más grande que su caja

El personaje se dibuja a 12×18 píxeles, pero su **caja de colisión sigue siendo 8×16**. El contorno sobresale de la caja:

```cpp
constexpr float kPlayerOffsetX = -2.0F;   // el dibujo empieza 2 px a la izquierda de la caja
constexpr float kPlayerOffsetY = -1.0F;
```

Esto es importante: **lo que ves y lo que colisiona son cosas distintas**. Podrías cambiar el dibujo por otro más grande sin alterar cómo se juega, y viceversa. Los pies quedan apoyados justo sobre el contorno del suelo, y la caja real (que es la que importa para las físicas) no cambia.

## Vegetación: decoración sin datos

Las matas, flores y enredaderas son **puramente cosméticas**: no afectan a la física ni a las monedas. Y aun así, queremos que cada sala se vea igual cada vez que entras.

La solución es elegir la decoración con una **función hash determinista** de la posición del tile:

```cpp
int standingDecoration(int column, int row) {
    const std::uint32_t slot = hashTile(column, row) % 12;
    if (slot >= 6) return kNoDecoration;       // la mitad de los tiles quedan sin nada
    return kDecorFirstStanding + slot;         // mata, mata, flor roja, flor amarilla, seta, arbusto
}
```

Dos ventajas frente a guardar la decoración en el mapa de Tiled:

1. **El diseñador del nivel no tiene que decorar**: dibuja el suelo y la vegetación aparece.
2. **Nada que sincronizar**: si cambias el mapa, la decoración se recalcula sola.

Es una función pura, así que tiene tests: es determinista, aparecen **todos** los tipos en un mapa grande, aproximadamente la mitad de los tiles quedan sin decorar y las enredaderas son escasas.

### Dónde se coloca

Para cada tile sólido:

- Si **no hay nada sólido encima**, puede crecer algo encima (se dibuja en la fila de arriba, solapando 1 píxel con el contorno).
- Si **no hay nada sólido debajo**, puede colgar una enredadera hacia abajo.

Por eso las plataformas flotantes tienen flores arriba y enredaderas abajo, y el suelo tiene vegetación solo por su cara superior.

También se eligen tiles distintos según lo que hay encima: un bloque de tierra con aire encima muestra **hierba**, y un ladrillo con aire encima muestra un ladrillo **con musgo**. Esa elección vive en `tileSheetIndex` y usa las mismas reglas de vecinos.

## El tileset ahora

```
tiles.png (32x8):   [tierra] [ladrillo] [tierra con hierba] [ladrillo con musgo]
```

En **Tiled** solo ves 4 tiles, pero en los mapas solo se usan los dos primeros (ids 1 y 2). Los otros dos los elige el renderer automáticamente. Así, cuando diseñes un nivel, solo piensas en "suelo" y "ladrillo".

## Cómo cambiar el arte

- **`assets/sprites/decor.png`**: 8 dibujos de 8×8 (6 que crecen hacia arriba y 2 enredaderas).
- **`assets/sprites/sprites.png`**: 4 poses del jugador (12×18) y 4 fotogramas de la moneda (8×8).
- **`assets/tiled/tiles.png`**: los 4 tiles.

Si añades un dibujo nuevo a `decor.png`, súbele el número de frames en `decoration.h` (`kDecorLastStanding`) y el test te dirá si todos aparecen.

## Cómo lo comprobé

Con el mismo truco de E7: un programa desechable que dibuja un frame en un **renderer de software** y lo guarda como imagen. Así pude revisar el aspecto, en reposo y saltando, sin abrir una ventana.

## Tests

`decoration_test`: determinismo, todos los tipos aparecen, la mitad queda vacía, las enredaderas son escasas y solo usan sus propios fotogramas.
