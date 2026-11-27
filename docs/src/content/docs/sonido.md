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

## La música de la cueva

`make_music.py` compone un bucle tranquilo de 32 segundos en La menor, sumando cuatro capas:

| Capa | Qué es |
|---|---|
| zumbido | dos notas muy graves casi iguales, que al chocar «laten» |
| colchón | acordes suaves que se funden uno en el siguiente |
| campanas | notas sueltas con el eco de la cueva |
| viento | ruido muy filtrado |

Para que **no se note dónde vuelve a empezar**, lo que suena más allá del final se suma al principio: el audio
se trata como un círculo.

<!-- src: game/tools/audio/make_music.py -->
```python
        buffer[(first + i) % LENGTH] += gain * shape(i / RATE, duration) * math.sin(step * i)
```

```bash
python3 game/tools/audio/make_music.py
afplay game/assets/music/cave.wav
```

Durante la partida, `-` y `+` cambian su volumen (de 0 a 10) y en pausa suena más baja. Cada paso aplica el
cuadrado de su fracción, porque el oído no percibe el volumen de forma lineal:

<!-- src: game/src/presentation/music_volume.cpp -->
```cpp
    const float share = static_cast<float>(step_) / static_cast<float>(kMusicSteps);
    return share * share * (paused ? kPausedMusicShare : 1.0F);
```

## Las gotas

Las [estalactitas de adorno](../efectos/) gotean. No hay ningún suceso del juego detrás, así que es el renderer
quien avisa cuando una gota que se ve **empieza a salpicar**, y `main` reproduce `drip`.

## La mezcla

Para que nada tape a la música, `main` fija cuánto suena cada cosa respecto a como se sintetizó:

<!-- src: game/src/main.cpp -->
```cpp
constexpr float kEffectsGain = 0.6F;
constexpr float kDripGain = 0.28F;  // the drops are scenery: quieter than what the player does
```

---

Hay una prueba que comprueba que cada sonido existe, se oye, no satura y dura como máximo 1,5 s.

Decisión: [ADR-0013](../decisiones/0013-sonido-por-eventos/).
