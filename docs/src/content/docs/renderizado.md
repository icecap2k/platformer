---
title: Cómo se dibuja
section: presentacion
order: 1
summary: El renderer, el orden de las capas y la diferencia entre lo que se ve y lo que choca.
---

`GameRenderer` **solo lee** la partida y la pinta. Nunca cambia nada del juego: puedes rehacer todo el aspecto
sin tocar una regla.

## Copiar rectángulos

Todo el dibujo es lo mismo: coger un trozo de una imagen (**origen**) y pegarlo en un sitio de la pantalla
(**destino**).

<!-- src: game/src/presentation/game_renderer.cpp -->
```cpp
    for (std::size_t index = 0; index < room.coins().size(); ++index) {
        if (session.collectedCoins().isCollected(room.id(), index)) {
            continue;
        }
        const domain::AABB box = room.coins()[index].bounds();
        sprites_.draw(renderer_, source, SDL_FRect{box.x, box.y, box.width, box.height});
    }
```

Se recorre el **dato** (las monedas de la sala), se consulta el **estado** (¿cogida?) y se pega.

## El orden de las capas

Lo que se dibuja después **tapa** a lo anterior (algoritmo del pintor):

<!-- src: game/src/presentation/game_renderer.cpp -->
```cpp
    drawCave();
    drawBackdrop(session.currentRoom(), time_ms);
    drawTiles(session.map());
    drawScenery(session.currentRoom(), time_ms);
    drawFireflies(session, time_ms);
    drawHazards(session);
    drawExits(session, time_ms);
    drawOrbs(session, time_ms);
    drawCheckpoints(session, time_ms);
    drawEnemies(session, time_ms);
    drawFruits(session, time_ms);
    drawCoins(session, time_ms);
    drawPlayer(session.player(), time_ms);
```

Fondo y setas lejanas, escenario, adornos, objetos y jugador. El marcador y la pausa se pintan después, encima.

## Lo que se ve no es lo que choca

| Elemento | Caja de colisión | Dibujo |
|---|---|---|
| Jugador | 8×16 | 32×28 |
| Escarabajo | 16×16 | 16×16 |
| Checkpoint | 8×8 | 12×20 |
| Salida | 8×8 | 22×32 |
| Orbe | 16×16 | 26×26 (con halo) |
| Pinchos (por tile) | 8×12 | 8×16 |

El dibujo se coloca **respecto a la caja**, casi siempre centrado y apoyado en su base:

<!-- src: game/src/presentation/game_renderer.cpp -->
```cpp
    const SDL_FRect destination{box.x + (box.width - kPlayerSpriteWidth) * 0.5F,
                                box.bottom() - kPlayerSpriteHeight + kPlayerSoleOverhang,
                                kPlayerSpriteWidth, kPlayerSpriteHeight};
```

## Transiciones con *easing*

Al cambiar de sala la anterior **se desliza** y la nueva entra por el lado por el que sales; al cambiar de
pantalla (título, juego, final) o al reaparecer, la nueva **se funde** sobre la anterior.

Para tener las dos imágenes a la vez, la escena no se pinta directamente en pantalla, sino en una textura
(`RenderTarget`). Cuando hay un cambio, se intercambian: la que se estaba viendo se guarda como "anterior".

<!-- src: game/src/presentation/game_renderer.cpp -->
```cpp
    scene_->begin();
    drawScene(session, flow, time_ms);
    scene_->end();
```

El **easing** hace que el movimiento arranque y frene suave en vez de ir a velocidad constante. Es una función
que convierte el tiempo transcurrido (de 0 a 1) en el avance (de 0 a 1):

<!-- src: game/src/presentation/transition.cpp -->
```cpp
float easeInOut(float t) {
    t = std::clamp(t, 0.0F, 1.0F);
    if (t < 0.5F) {
        return 4.0F * t * t * t;
    }
    const float rest = 2.0F - 2.0F * t;
    return 1.0F - rest * rest * rest * 0.5F;
}
```

Al principio `t³` crece muy despacio; en la segunda mitad se usa la misma curva al revés. Mientras dura la
transición (450 ms el deslizamiento, 600 ms el fundido) **el juego se detiene**: ver [El bucle](../bucle/).

## La pantalla de título

Se dibuja aparte (`drawTitle`): la imagen del autor a pantalla completa, el nombre sobre el cielo y el menú sobre
un panel oscuro a la izquierda, para no tapar a la protagonista.

## Texto y menús

El texto usa la fuente de 8×8 que trae SDL (`SDL_RenderDebugText`), ampliada para los títulos. Por eso los
rótulos del juego van en inglés y sin acentos.

Siguiente: [gráficos y sprites](../graficos/).
