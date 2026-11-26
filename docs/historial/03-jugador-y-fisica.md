---
title: "El jugador, la gravedad y el salto (física testeable)"
epic: "E3 · Jugador y física"
date: 2026-11-17
summary: "Un Player con caja de colisión, gravedad, salto y una GameSession que lo orquesta, todo probado sin abrir ventana."
---

Por fin aparece el protagonista. Lo importante de este hito no es el rectángulo amarillo que ves en pantalla, sino **dónde vive cada cosa** y cómo se prueba la física sin gráficos.

## Reparto de responsabilidades

```
domain/entities/player.h        datos del jugador (posición, velocidad, tamaño)
domain/physics/aabb.h           caja de colisión
domain/physics/player_physics   reglas: cómo se mueve en un paso
application/game_session        orquesta: recibe input, avanza el estado
presentation/game_renderer      dibuja el estado (solo lectura)
main.cpp                        conecta todo
```

Una diferencia con muchos juegos pequeños: `Player` **no tiene un método `update()`**. Es un struct con datos, y las reglas están en una función aparte, `stepPlayer`. Es como separar un *value object* de un *servicio de dominio*: los datos son fáciles de crear en un test, y las reglas se pueden cambiar sin tocar la entidad.

## AABB: la caja de colisión

AABB significa *axis-aligned bounding box*: un rectángulo que nunca rota. Es la forma más simple y más usada en plataformas 2D.

```cpp
bool overlaps(const AABB& other) const {
    return left() < other.right() && right() > other.left() &&
           top() < other.bottom() && bottom() > other.top();
}
```

Dos cajas se solapan si se solapan **en los dos ejes a la vez**. Fíjate en que usamos `<` y `>` estrictos: dos cajas que solo se tocan por un borde *no* cuentan como colisión, lo que evita que el jugador se quede "pegado" al suelo.

Recuerda que en pantalla la **Y crece hacia abajo**: `top()` es menor que `bottom()`.

## Un paso de física

Todas las unidades son **píxeles por paso**, y un paso dura 1/60 s (el `FixedTimestep` de E1). Por eso no hay ningún `delta_time` en las fórmulas: el paso es siempre el mismo.

```cpp
void stepPlayer(Player& player, const InputState& input, const FlatArena& arena) {
    // 1. Horizontal: control directo, sin aceleración
    player.velocity_x = input.horizontalAxis() * kRunSpeed;
    player.x += player.velocity_x;

    // 2. Salto: solo si está apoyado
    if (input.jump && player.on_ground) {
        player.velocity_y = -kJumpSpeed;
        player.on_ground = false;
    }

    // 3. Gravedad con velocidad terminal
    player.velocity_y = std::min(player.velocity_y + kGravity, kMaxFallSpeed);
    player.y += player.velocity_y;

    // 4. Aterrizaje
    // ...
}
```

Los valores (`kGravity = 0.2`, `kJumpSpeed = 5.0`) son los de tu `demo5`. Con ellos, el salto sube unos **62 píxeles**: de física de instituto, `v² / (2g) = 25 / 0.4 = 62.5`. Y como eso es una fórmula, podemos *comprobarla en un test*:

```cpp
// v^2 / (2g) = 62.5 px, give or take the discrete integration.
CHECK(floor_top - highest == doctest::Approx(62.5F).epsilon(0.1));
```

Si mañana cambias la gravedad, este test te avisa de que el salto ya no es el que diseñaste.

### Por qué solo se salta desde el suelo

`input.jump && player.on_ground`. Sin esa condición, mantener pulsado el salto haría volar al personaje. Hay un test específico que lo protege ("cannot jump while already in the air").

### Un mundo provisional: `FlatArena`

Todavía no tenemos mapas, así que el mundo es un suelo plano a una altura y dos paredes invisibles en los laterales. Es un tipo **provisional**: en E4 lo sustituiremos por el mapa de tiles de verdad, y `stepPlayer` dejará de recibir una arena para recibir el mapa.

## `GameSession`: la capa de aplicación

```cpp
class GameSession {
public:
    void update(const domain::InputState& input);
    const domain::Player& player() const;
    // ...
};
```

Es el único punto de entrada al juego para las capas externas: la infraestructura le da el input, la presentación lee su estado para dibujar. No usa SDL, así que se puede simular una partida entera en un test:

```cpp
for (int i = 0; i < 200; ++i) {
    session.update(InputState{});
}
CHECK(session.player().on_ground);
```

Ahora mismo `GameSession` hace poco (llama a `stepPlayer`), pero irá creciendo: en E5 gestionará el cambio de pantalla y en E6 las monedas.

## La capa `presentation`

`drawGame` recibe la sesión **como `const`**: puede leer todo, pero no cambiar nada. Dibuja un fondo, un suelo y el jugador como un rectángulo. Sin sprites todavía; eso llega en E7.

Con esto, el `main.cpp` queda muy corto:

```cpp
for (int i = 0; i < steps; ++i) {
    session.update(input);
}
presentation::drawGame(window.renderer(), session);
```

## Mapa de dependencias actual

```
domain          ← Player, AABB, stepPlayer, InputState, FixedTimestep
application     ← GameSession                    (usa domain)
infrastructure  ← Window, KeyboardInputMapper    (usa domain + SDL3)
presentation    ← drawGame                       (usa application + SDL3)
tests           ← solo domain y application, nunca SDL
```

## Cómo probarlo

```bash
cmake -S game -B build && cmake --build build
ctest --test-dir build --output-on-failure
./build/platformer
```

El personaje cae desde el aire al arrancar. Muévelo con flechas o `A`/`D` y salta con `Espacio`, `W` o flecha arriba. Si mantienes el salto pulsado, volverá a saltar en cuanto aterrice; más adelante podremos exigir soltar y volver a pulsar.

## Siguiente: E4

Sustituimos la arena por un **mapa de tiles** cargado desde un fichero de Tiled, y la física pasa a colisionar con plataformas de verdad, resolviendo cada eje por separado.
