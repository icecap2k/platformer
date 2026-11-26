---
title: Objetos del nivel
section: dominio
order: 6
summary: Monedas, frutas, orbes, checkpoints y la salida.
---

| Objeto | Tamaño | Al tocarlo | ¿Se conserva al reaparecer? |
|---|---|---|---|
| `Coin` | 8×8 | suma al contador | sí |
| `Fruit` | 8×8 | cura 1 corazón (aunque estés al máximo) | sí (no reaparece) |
| `Orb` | 16×16 | cuenta para abrir la salida | sí |
| `Checkpoint` | 8×8 | pasa a ser el punto de reaparición | sí |
| `Exit` | 8×8 | gana la partida, **si está abierta** | — |

Todos son datos sencillos con una función `bounds()`.

## Recoger: una sola función para todos

Como coincide la forma (`bounds()`), una **plantilla** sirve para cualquier recogible:

<!-- src: game/src/domain/rules/coin_pickup.h -->
```cpp
template <class Item>
std::vector<std::size_t> pickupsTouchedBy(const Player& player, const RoomId& room,
                                          const std::vector<Item>& items,
                                          const PickupCollection& collection) {
    std::vector<std::size_t> touched;
    const AABB body = player.bounds();
    for (std::size_t index = 0; index < items.size(); ++index) {
        if (!collection.isCollected(room, index) && body.overlaps(items[index].bounds())) {
            touched.push_back(index);
        }
    }
    return touched;
}
```

`template <class Item>` significa "esta función vale para cualquier tipo `Item` que tenga lo que uso dentro".
Es el equivalente a un genérico de TypeScript.

La función **solo dice cuáles se tocan**. Apuntarlos como recogidos y avisar es tarea de la
[partida](../game-session/). Separar "detectar" de "aplicar" hace cada parte fácil de probar.

## La salida

La salida está cerrada hasta tener todos los orbes:

<!-- src: game/src/application/game_session.h -->
```cpp
    // The exit only works once every orb has been taken (at once in a world without orbs).
    bool exitOpen() const { return collected_orbs_.count() >= world_.totalOrbs(); }
```

Decisiones: [ADR-0012](../decisiones/0012-frutas-que-curan-y-recogibles/),
[ADR-0015](../decisiones/0015-checkpoints/) y [ADR-0021](../decisiones/0021-orbes-que-abren-la-salida/).
