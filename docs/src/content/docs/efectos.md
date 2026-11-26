---
title: Decoración y efectos
section: presentacion
order: 5
summary: Adornos, gotas, luciérnagas y la sacudida de pantalla, sin tocar las reglas.
---

Nada de esto afecta al juego. La regla del proyecto: un efecto es una **función de la sala, la posición y el
reloj**. No guarda estado ni se escribe en los mapas, pero se ve igual cada vez.

## El truco: un hash

Un *hash* convierte datos en un número que parece aleatorio pero es **siempre el mismo** para la misma entrada:

<!-- src: game/src/presentation/decoration.cpp -->
```cpp
    std::uint32_t h = static_cast<std::uint32_t>(column) * 73856093U ^
                      static_cast<std::uint32_t>(row) * 19349663U ^ seed;
    h ^= h >> 13;
```

## La decoración

![Los 16 adornos](/img/sheets/decor.png)

`sceneryFor(sala)` recorre las casillas y, según el hash, intenta plantar un adorno en el suelo, el techo o una
pared. Solo se acepta si:

- sus casillas están vacías y no pisa otro adorno,
- tiene roca donde apoyarse,
- no tapa pinchos, objetos, la salida ni el marcador.

Los colores están **acercados al fondo** a propósito: una decoración brillante se confundía con los pinchos.

## Gotas

Cada racimo de estalactitas tiene su ritmo. `dripAt` calcula, para un instante, si hay gota y en qué fase:
formándose, cayendo o salpicando. La caída usa la fórmula de la gravedad, `d = ½·g·t²`:

<!-- src: game/src/presentation/drip.cpp -->
```cpp
        drip.fall_offset = std::fmin(0.5F * kDropGravity * seconds_falling * seconds_falling, fall_distance);
```

## Luciérnagas

Tres por sala. Cada una repite: quieta 3 s, vuelo en curva, quieta 3 s y vuelta. La curva sale de elevar al
cuadrado el avance: empieza plana y termina empinada.

<!-- src: game/src/presentation/firefly.cpp -->
```cpp
    const float smooth = along * along * (3.0F - 2.0F * along);
    Firefly firefly;
    firefly.x = static_cast<float>(route.column) * size + across * smooth;
    firefly.y = static_cast<float>(route.row) * size + down * along * along;
```

(`smooth` es un *suavizado*: arranca y frena despacio.)

## Sacudida de pantalla

Al recibir un golpe la imagen tiembla 14 pasos. No hace falta un temporizador propio: se deduce de la
invulnerabilidad que ya tiene el jugador.

<!-- src: game/src/presentation/screen_shake.cpp -->
```cpp
    const int since_hit = invulnerable_total - invulnerable_steps;
```

Decisión de fondo: [ADR-0008](../decisiones/0008-efectos-visuales-puros/).
