---
title: Audio
section: infraestructura
order: 3
summary: Un reproductor que solo sabe de nombres de ficheros y nunca rompe el juego.
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

## Degradar con elegancia

Si no hay tarjeta de sonido o falta un fichero, avisa **una vez** y sigue en silencio. El juego no puede
fallar por el audio. Por eso el constructor no lanza excepciones, al contrario que el cargador de mapas:
un mapa es imprescindible y un sonido no.

Los ficheros se **sintetizan** con `game/tools/audio/make_sounds.py`: ver [Sonido](../sonido/).
