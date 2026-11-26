---
title: "Mapas de tiles, Tiled y colisiones por ejes"
epic: "E4 · Tiles y colisiones"
date: 2026-11-17
summary: "Sustituimos el suelo provisional por un mapa de tiles cargado desde Tiled, con colisiones resueltas eje por eje."
---

En E3 el mundo era un suelo plano y dos paredes invisibles. Ahora el nivel es un **mapa de tiles**: una cuadrícula donde cada celda es "vacío" o "sólido". Es la forma estándar de construir niveles de plataformas y, además, hace que el nivel sea *datos* y no código.

## `TileMap`: el mundo en el dominio

```cpp
class TileMap {
public:
    TileMap(int width, int height, int tile_size, std::vector<int> tiles);
    int tileAt(int tile_x, int tile_y) const;
    bool isSolid(int tile_x, int tile_y) const { return tileAt(tile_x, tile_y) != 0; }
};
```

Tres decisiones de diseño:

- **Los tiles son enteros; 0 significa vacío.** Es la convención de Tiled y la mantenemos. Por ahora todo lo no-cero es sólido, pero `isSolid` es el único sitio que lo decide: si mañana hay plataformas de un solo sentido o pinchos, se cambia ahí.
- **Fuera de la cuadrícula está vacío.** Así la física no necesita comprobar límites en cada consulta.
- **El constructor valida.** Si la lista de tiles no tiene `ancho × alto` elementos, lanza `std::invalid_argument`. Un `TileMap` que existe siempre es consistente (como un *value object* con invariantes).

## Colisiones: un eje cada vez

Esta es la idea más importante del hito. Si mueves al jugador en X e Y a la vez y chocas con una esquina, no sabes si fue una pared o un suelo. La solución es **separar los ejes**:

```cpp
moveHorizontally(player, map);   // 1. mover en X y resolver choques con paredes
// ... salto y gravedad ...
moveVertically(player, map);     // 2. mover en Y y resolver suelo / techo
```

Al mover solo en X, un choque *solo puede ser una pared*. Al mover solo en Y, *solo puede ser suelo o techo*. Cada caso se resuelve igual de simple:

```cpp
// Moviéndose hacia abajo: comprobamos la fila que toca el pie
const int row = lastTileIndexBefore(player.y + player.height, tile_size);
if (anySolidInRow(map, row, first_column, last_column)) {
    player.y = row * tile_size - player.height;   // colocar justo encima del tile
    player.velocity_y = 0.0F;
    player.on_ground = true;
}
```

En vez de revisar todos los tiles del mapa, solo miramos los que toca el **borde que avanza**: el pie si cae, la cabeza si sube, el costado si camina. Y al chocar, se recoloca al jugador pegado a la cara del tile.

### El detalle del borde exacto

¿En qué tile está un jugador cuyo pie está justo en `y = 216`, con tiles de 8 píxeles? 216 / 8 = 27 exacto. El pie está en el *límite* entre la fila 26 y la 27, pero ocupa la 26. Por eso usamos:

```cpp
int lastTileIndexBefore(float end_pixel, int tile_size) {
    return ceil(end_pixel / tile_size) - 1;
}
```

Un `floor` aquí daría 27 y el jugador "colisionaría" con el suelo mientras está de pie sobre él. Es el típico fallo de uno-de-más (*off-by-one*), y los tests lo vigilan.

### Una limitación consciente: el *tunneling*

Si el jugador fuera a más de un tile por paso (8 px), podría atravesar un tile fino sin que se detecte, porque solo miramos la posición final. Nuestras velocidades máximas son 1,5 px/paso en horizontal y 6 px/paso cayendo, así que estamos a salvo. Lo dejamos escrito en el comentario de `stepPlayer`, para que quien suba la velocidad lo sepa.

## Tests sin ventana

Montamos pequeños mapas en memoria y comprobamos cada comportamiento:

```cpp
SUBCASE("a wall blocks horizontal movement") {
    const TileMap map = makeMap({{20, 26}, {20, 25}, {20, 24}});  // wall at column 20
    Player player = standingAt(100.0F, kFloorTop);
    // ... run right for 200 steps ...
    CHECK(player.x + player.width == doctest::Approx(160.0F));  // wall's left face
}
```

Hay pruebas para aterrizar sobre una plataforma, caminar y caerse de ella, chocar con una pared por ambos lados, golpearse la cabeza contra un techo y no atravesar una plataforma saltando desde abajo.

## Tiled: el editor de niveles

[Tiled](https://www.mapeditor.org/) es un editor gratuito de mapas. Guarda en `.tmj` (JSON), que es fácil de leer. Nuestro `TiledMapLoader` hace tres cosas:

1. Lee `width`, `height` y `tilewidth` del mapa.
2. Coge la **primera capa de tiles** y su array `data`.
3. Limpia cada id con una máscara: Tiled guarda en los bits altos si el tile está volteado o rotado, y por ahora los ignoramos.

```cpp
constexpr std::uint32_t kTileIdMask = 0x1FFFFFFFU;
tiles.push_back(static_cast<int>(raw.get<std::uint32_t>() & kTileIdMask));
```

Para leer el JSON reutilizamos tu `json.hpp` (nlohmann), el mismo de Coffee Crisis, copiado en `game/third_party/`. Si el mapa está mal formado (no es JSON, falta la capa, el tamaño no cuadra, usa codificación base64), el loader lanza un `std::runtime_error` con un mensaje legible en vez de romper más adelante.

### Separar la lectura de ficheros del parseo

Hay dos funciones:

```cpp
TileMap parseTiledMap(const std::string& json_text);  // pura, testeable con strings
TileMap loadTiledMap(const std::string& path);        // lee el fichero y llama a la anterior
```

Así los tests de formato no necesitan ficheros: pasan el JSON como texto. Y además hay un test que carga el `room_01.tmj` real, para que si alguien rompe el nivel, los tests lo detecten.

### Una librería nueva sin SDL

Cargar mapas no necesita SDL, así que el loader tiene su propia librería de CMake, `infrastructure_assets`. Los tests pueden enlazarla sin arrastrar SDL. Sigue siendo "infraestructura" por su carpeta y por lo que hace (tocar el disco), pero no se ensucia con la ventana.

## El primer nivel

`game/assets/tiled/room_01.tmj` es un mapa de 40×30 tiles (320×240 px) con suelo, cuatro plataformas a distintas alturas y un muro pequeño. Junto a él hay `tiles.png`, un tileset de 2 tiles (suelo y ladrillo) para que Tiled pueda abrirlo; el juego aún no usa la imagen, dibuja rectángulos de color, y los sprites llegan en E7.

Para editarlo, abre `room_01.tmj` con Tiled. Cada vez que compiles, CMake copia la carpeta `assets/` junto al ejecutable, así que el juego recoge tus cambios.

## `GameSession` ahora recibe el mapa

```cpp
application::GameSession session(
    infrastructure::loadTiledMap(assetPath("tiled/room_01.tmj")), kSpawnX, kSpawnY);
```

`FlatArena` ha desaparecido. La sesión es dueña del `TileMap` y la capa de presentación lo recorre para dibujar los tiles. Fíjate en cómo `main.cpp` sigue siendo el único sitio que conoce a todas las capas.

## Mapa de dependencias

```
domain                ← TileMap, Player, stepPlayer (sin SDL)
application           ← GameSession
infrastructure_assets ← TiledMapLoader (domain + json, sin SDL)
infrastructure        ← Window, KeyboardInputMapper (+ SDL3)
presentation          ← drawGame (+ SDL3)
```

## Cómo probarlo

```bash
cmake -S game -B build && cmake --build build
ctest --test-dir build --output-on-failure
./build/platformer
```

Salta sobre las plataformas, golpéate la cabeza desde debajo, choca con el muro de la izquierda (a la izquierda del punto de aparición) y camina hasta el borde de una plataforma para caer.

## Siguiente: E5

El gran hito: **varias pantallas conectadas**. Cuando el jugador salga por cualquiera de los cuatro bordes, cargará la sala vecina y aparecerá por el lado opuesto.
