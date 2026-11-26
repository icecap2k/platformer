---
title: Sonido
section: presentacion
order: 6
summary: De un suceso del juego a un fichero que suena, y cómo se sintetizan.
---

El recorrido completo:

```
GameSession ──evento──► soundFor ──Sound──► soundName ──"coin"──► AudioPlayer
 (aplicación)           (presentación)                           (infraestructura)
```

## De evento a sonido

<!-- src: game/src/presentation/sound_cues.cpp -->
```cpp
            [](const RoomChanged&) -> std::optional<Sound> { return std::nullopt; },
            [](const CoinCollected&) -> std::optional<Sound> { return Sound::Coin; },
```

Se usa `std::visit` sobre el `variant` de eventos. Su gran ventaja: **si añades un evento y olvidas decidir su
sonido, no compila**.

La sintaxis `[](const CoinCollected&) { ... }` es una **función anónima** (*lambda*), como una función flecha.

## Sintetizar los sonidos

`game/tools/audio/make_sounds.py` crea cada `.wav` a partir de pocos ingredientes:

| Ingrediente | Suena a |
|---|---|
| onda cuadrada | videojuego clásico |
| onda triangular o senoidal | notas suaves |
| ruido filtrado | viento, golpes, roce |
| barrido de frecuencia | saltos, "boing" |
| envolvente | cómo nace y muere el sonido |
| resonador | timbre metálico (la reja) |

El de la moneda son dos notas seguidas:

<!-- src: game/tools/audio/make_sounds.py -->
```python
    return mix(note(988, 0.06), delayed(note(1319, 0.14), 0.06))
```

```bash
python3 game/tools/audio/make_sounds.py            # escribe los .wav
python3 game/tools/audio/make_sounds.py --report   # duración y volumen, sin escribir
afplay game/assets/sounds/coin.wav                 # escuchar uno (macOS)
```

Hay una prueba que comprueba que cada sonido existe, se oye, no satura y dura como máximo 1,5 s.

Decisión: [ADR-0013](../decisiones/0013-sonido-por-eventos/).
