---
title: "Sprites, tilesets y animación sin mezclar capas"
epic: "E7 · Pulido (1/2)"
date: 2026-11-17
summary: "Sustituimos los rectángulos por arte de verdad. La decisión de qué dibujo mostrar es lógica pura y se testea; dibujarlo es SDL."
---

Hasta ahora todo eran rectángulos de colores. En este hito el juego cobra aspecto: un jugador con gorra que camina y salta, monedas que giran, suelo con hierba y ladrillos. Lo interesante, otra vez, es **cómo se reparte** el trabajo entre capas.

## El arte

Dos imágenes PNG, ambas con **transparencia** (RGBA):

```
assets/tiled/tiles.png      24x8    3 tiles de 8x8: tierra, ladrillo, hierba
assets/sprites/sprites.png  32x24   fila 1: 4 poses del jugador (8x16)
                                    fila 2: 4 fotogramas de la moneda (8x8)
```

Las generé con un script, dibujando cada sprite como una cuadrícula de caracteres (`H` = gorra, `S` = piel, `B` = camiseta...). Para un juego con arte propio usarías un editor como Aseprite o Piskel, y guardarías el PNG igual.

Una **hoja de sprites** (*sprite sheet*) junta muchas imágenes pequeñas en un fichero. Se carga una sola textura y, para dibujar, se elige qué rectángulo recortar (`source`) y dónde pintarlo (`destination`).

## Cargar imágenes: `Texture`

SDL3 por sí solo no sabe leer PNG. Usamos `stb_image`, el mismo que ya tenías en Coffee Crisis (copiado a `game/third_party/stb`):

```cpp
unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);  // fuerza RGBA
SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, pixels, width * 4);
texture_ = SDL_CreateTextureFromSurface(renderer, surface);
```

Pasos: decodificar a píxeles en memoria → envolverlos en una *superficie* (memoria de la CPU) → convertirlos en *textura* (memoria de la GPU). Después se liberan los píxeles originales.

`Texture` es una clase RAII, como `Window`: el destructor libera la textura de SDL y el constructor lanza una excepción con un mensaje claro si el fichero no existe. Además activa `SDL_SCALEMODE_NEAREST`, que hace que al ampliar no se difuminen los píxeles.

## Qué dibujo mostrar: lógica pura

¿Cuándo está el jugador saltando, caminando o parado? Eso es una decisión, y las decisiones se testean. La separamos en `presentation/animation.*`, **sin ninguna dependencia de SDL**:

```cpp
PlayerPose playerPose(const domain::Player& player, std::uint64_t time_ms) {
    if (!player.on_ground)        return PlayerPose::Jump;
    if (player.velocity_x == 0)   return PlayerPose::Idle;
    return ((time_ms / 120) % 2 == 0) ? PlayerPose::Walk1 : PlayerPose::Walk2;
}
```

Las reglas son: en el aire, pose de salto; parado, reposo; caminando, **alternar dos poses cada 120 ms**. El tiempo no se lee dentro de la función: se **le pasa**, igual que `FixedTimestep` recibía el tiempo transcurrido en E1. Así los tests controlan el tiempo a su antojo:

```cpp
CHECK(presentation::playerPose(walking, 119) == PlayerPose::Walk1);
CHECK(presentation::playerPose(walking, 120) == PlayerPose::Walk2);
```

Hay un detalle: la dirección en la que mira. Si el personaje se detiene, debe seguir mirando hacia donde iba. Como el dominio no sabe de eso (la dirección es *cosmética*, no afecta a la física), `facesLeft(player, was_facing_left)` recibe la dirección anterior y la conserva cuando no hay movimiento. La memoria de esa dirección vive en el renderer, no en el dominio.

Esto responde a una pregunta típica: *¿dónde va el estado que solo importa visualmente?* En la capa de presentación. El dominio no se entera de si el jugador mira a izquierda o derecha.

Como la lógica de animación no usa SDL, tiene su propia librería de CMake (`presentation_animation`) que los tests enlazan, igual que hicimos con `infrastructure_assets`.

## Dibujar: `GameRenderer`

La función libre `drawGame` se convierte en una clase:

```cpp
class GameRenderer {
public:
    GameRenderer(SDL_Renderer* renderer, const std::string& assets_dir);
    void draw(const application::GameSession& session);
private:
    infrastructure::Texture tiles_;
    infrastructure::Texture sprites_;
    bool facing_left_ = false;
};
```

¿Por qué una clase? Porque ahora tiene **estado propio**: las texturas cargadas y la dirección. El estado de la partida sigue en `GameSession`, que se recibe como `const`: el renderer lo lee, nunca lo cambia.

### Autotiling mínimo

El mapa de Tiled solo dice "aquí hay tierra" (tile 1). Pero queremos hierba **solo en la capa superior** del suelo, no en todos los bloques. El renderer decide:

```cpp
if (tile_id == 1) {
    return map.isSolid(column, row - 1) ? kTileDirt : kTileGrass;
}
```

Si encima hay otro bloque sólido, tierra; si encima hay aire, hierba. Es una versión muy simple del *autotiling* de los editores de niveles. Lo importante es que **el diseñador del nivel solo dibuja "suelo"** y el aspecto se deduce, no se repite a mano.

### Otras mejoras visuales

- **Fondo degradado** hecho con 30 franjas horizontales que van de azul oscuro a azul más claro.
- **HUD con icono**: la moneda animada junto al contador `0/3`.
- La moneda gira con 4 fotogramas (el tercero es una moneda "de canto") y su ciclo es `coinFrame(time_ms)`, también puro y testeado.

## Cómo comprobé que se ve bien

No podía mirar la ventana, así que escribí un programa desechable que crea un **renderer de software sobre una superficie en memoria**, ejecuta la sesión unos pasos, dibuja un frame y lo guarda como imagen. Con `SDL_VIDEODRIVER=dummy` ni siquiera necesita pantalla. Es una técnica muy útil para revisar el aspecto de un juego automáticamente: el mismo código de `GameRenderer` dibuja en un buffer en vez de en una ventana, porque solo habla con un `SDL_Renderer*` y no le importa de dónde viene.

## Tests

`animation_test`: reposo, salto en el aire, ciclo de caminar a lo largo del tiempo, caminar a la izquierda, mantener la dirección al parar y el ciclo de 4 fotogramas de la moneda.

## Editar el arte

Abre `tiles.png` o `sprites.png` en cualquier editor de píxeles. Si añades tiles al tileset, tendrás que extender `tileSheetIndex` en `game_renderer.cpp` para que el renderer los conozca (por ahora solo sabe de tierra, ladrillo y hierba).

## Siguiente

Un último post con el **mapa completo de la arquitectura** y por dónde seguir: mando, sonido, enemigos.
