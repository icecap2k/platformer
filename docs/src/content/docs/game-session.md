---
title: La partida (GameSession)
section: aplicacion
order: 1
summary: La puerta de entrada a las reglas; avanza un paso y cuenta qué ha pasado.
---

`GameSession` es **una partida**: tiene el mundo, el jugador y todo el estado. Lo de fuera solo necesita dos
cosas: darle la entrada y leer cómo está.

<!-- src: game/src/application/game_session.h -->
```cpp
    // Advances one step and returns what happened during it (usually nothing).
    // Once the game is won it no longer advances (and returns nothing).
    std::vector<GameEvent> update(const domain::InputState& input);
```

No usa SDL: una prueba puede jugar partidas enteras en memoria.

## El orden de un paso

1. Mover al jugador (física) y cerrar los bordes sin vecino.
2. Anunciar salto, aterrizaje o deslizamiento.
3. ¿Ha cruzado un borde? Cambiar de sala.
4. Recoger monedas, frutas, orbes y checkpoints.
5. La salida: avisar si su reja debe subir y, si está abierta y la toca, ganar.
6. Daño: pinchos, enemigos y estalactitas. Si muere, reaparecer o reiniciar.

El orden importa: por ejemplo, los objetos se miran **en la sala en la que está ahora**, que puede haber
cambiado en el punto 3.

## Eventos: contar en vez de actuar

La partida no reproduce sonidos ni escribe mensajes. **Devuelve una lista de sucesos:**

<!-- src: game/src/application/game_session.cpp -->
```cpp
    for (const std::size_t index : domain::coinsTouchedBy(player_, here, collected_coins_)) {
        collected_coins_.collect(here.id(), index);
        events.push_back(CoinCollected{here.id(), index, collected_coins_.count()});
    }
```

Cada suceso es un `struct` y la lista admite cualquiera de ellos (`std::variant`):

<!-- src: game/src/application/game_event.h -->
```cpp
using GameEvent = std::variant<RoomChanged, CoinCollected, FruitEaten, PlayerJumped, PlayerLanded,
                               SlideStarted, PlayerHurt, StalactiteWarned, StalactiteFell,
                               StalactiteShattered, EnemyStomped, CheckpointReached, PlayerRespawned,
                               OrbCollected, GateOpened, GameWon, GameRestarted>;
```

Así el juego dice **qué ha pasado** y otros deciden **qué hacer**: hoy, [sonidos](../sonido/) y mensajes en
consola; mañana, partículas, sin tocar las reglas. Es lo que en DDD llamarías *eventos de dominio*.

## Reaparecer y reiniciar

<!-- src: game/src/application/game_session.cpp -->
```cpp
    if (last_checkpoint_) {
        respawnAtCheckpoint();
        events.push_back(PlayerRespawned{current_room_});
    } else {
        restart();
        events.push_back(GameRestarted{});
    }
```

Como el [estado está separado de los datos](../dominio-vision/), reiniciar es vaciar unas pocas colecciones.

Siguiente: [los menús](../menus/).
