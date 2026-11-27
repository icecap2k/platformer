---
title: Audio
section: infraestructura
order: 3
summary: Un reproductor que solo sabe de ficheros, con efectos y música en bucle, y que nunca rompe el juego.
---

`AudioPlayer` no sabe nada del juego: le dicen un **nombre** y reproduce `assets/sounds/<nombre>.wav`. Qué
sonido corresponde a cada suceso lo decide la presentación ([Sonido](../sonido/)).

Cada sonido tiene su propio **flujo** (*stream*) conectado al mismo dispositivo: así suenan varios a la vez y
repetir uno lo reinicia.

<!-- src: game/src/infrastructure/audio/audio_player.cpp -->
```cpp
    SDL_ClearAudioStream(found->second.stream);  // start from the beginning
    SDL_PutAudioStreamData(found->second.stream, found->second.data, static_cast<int>(found->second.length));
```

## La música en bucle

La música es un clip más, pero en lugar de reiniciarse se **vuelve a poner en cola** antes de acabarse. `main`
llama a `update()` en cada fotograma:

<!-- src: game/src/infrastructure/audio/audio_player.cpp -->
```cpp
    if (music_.stream != nullptr && SDL_GetAudioStreamQueued(music_.stream) < static_cast<int>(music_.length)) {
        SDL_PutAudioStreamData(music_.stream, music_.data, static_cast<int>(music_.length));
    }
```

Tiene su propio volumen (la ganancia de su flujo), aparte del de los efectos.

## Degradar con elegancia

Si no hay tarjeta de sonido o falta un fichero, avisa **una vez** y sigue en silencio. El juego no puede
fallar por el audio. Por eso el constructor no lanza excepciones, al contrario que el cargador de mapas:
un mapa es imprescindible y un sonido no.

Los ficheros se **sintetizan** con `game/tools/audio/make_sounds.py`: ver [Sonido](../sonido/).
