---
title: C++ para novatos
section: empezar
order: 2
summary: Las construcciones de C++ que aparecen en este proyecto, con ejemplos suyos.
---

No hace falta saber todo C++ para leer este código. Esto es lo que se repite, explicado con ejemplos reales.

## `.h` y `.cpp`

El `.h` (cabecera) **declara** qué existe; el `.cpp` **define** cómo funciona. Quien quiere usar algo hace
`#include` de la cabecera. `#pragma once` evita incluirla dos veces. Se parece a separar la interfaz pública de
la implementación.

## `namespace`

Agrupa nombres. `domain::Player` es "el `Player` del dominio". En este proyecto cada capa tiene el suyo.

## `struct` y `class`

Son lo mismo salvo la visibilidad por defecto. Por convención aquí:

- `struct` = **datos** sencillos, todo público:

<!-- src: game/src/domain/world/orb.h -->
```cpp
struct Orb {
    float x = 0.0F;  // top-left corner of its hitbox
    float y = 0.0F;
    float size = 16.0F;
    OrbColour colour = OrbColour::Red;

    AABB bounds() const { return AABB{x, y, size, size}; }
};
```

- `class` = algo que **protege sus datos** y ofrece métodos:

<!-- src: game/src/domain/world/coin_collection.h -->
```cpp
class PickupCollection {
public:
    bool isCollected(const RoomId& room, std::size_t index) const;

    // Returns true only the first time; collecting twice changes nothing.
    bool collect(const RoomId& room, std::size_t index);

    int count() const { return static_cast<int>(collected_.size()); }

private:
    std::set<std::pair<RoomId, std::size_t>> collected_;
};
```

Los nombres con guion bajo final (`collected_`) son miembros privados. `0.0F` es un número `float`.

## `const` y `&`

- `const Room& room`: recibo **una referencia** (no una copia) y prometo **no modificarla**.
- `Player& player`: recibo una referencia y **sí** la modifico.
- `int count() const`: este método no cambia el objeto.

Si ves una función que recibe `Player&`, ya sabes que puede cambiar al jugador.

## `enum class`

Una lista cerrada de opciones, con su propio nombre delante: `Screen::Title`, `Side::North`.

## `std::vector`, `std::map`, `std::optional`

| Tipo | Qué es | En JavaScript sería |
|---|---|---|
| `std::vector<Coin>` | lista que crece | un array |
| `std::map<RoomId, Room>` | clave → valor, ordenado | un `Map` |
| `std::optional<Side>` | puede haber valor o no | `Side \| undefined` |

Un `optional` se pregunta antes de usarlo:

<!-- src: game/src/application/game_session.cpp -->
```cpp
    if (const auto side = domain::exitedThrough(player_, room.map())) {
        const domain::RoomId previous = current_room_;
        current_room_ = *room.neighbor(*side);
```

Dentro del `if`, `*side` saca el valor. `auto` pide al compilador que deduzca el tipo.

## `constexpr`

Un valor conocido al compilar. Las constantes de ajuste del juego son así, con una `k` delante:

<!-- src: game/src/domain/physics/player_physics.h -->
```cpp
inline constexpr float kRunSpeed = 1.5F;
inline constexpr float kGravity = 0.2F;
```

## `static_cast`

Conversión explícita entre tipos, por ejemplo de entero a decimal: `static_cast<float>(column)`. Es verboso a
propósito: se ve dónde se convierte.

## `std::variant`

"Uno de estos tipos". Los [eventos del juego](../game-session/) son un `variant`: cada suceso es un `struct`
distinto y quien escucha debe contemplar todos.

## RAII

La idea más "de C++": un objeto **coge un recurso en su constructor y lo suelta en su destructor**. Cuando el
objeto deja de existir, la limpieza ocurre sola. Así funcionan [`Window`, `Texture` y `AudioPlayer`](../sdl/).

## Dónde seguir

[Las capas](../capas/).
