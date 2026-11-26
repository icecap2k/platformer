---
title: "Monedas, estado persistente y eventos"
epic: "E6 · Coleccionable"
date: 2026-11-17
summary: "Una moneda en algunas salas que se recoge al tocarla y no reaparece. Separamos datos del nivel, estado de la partida y eventos."
---

Con las pantallas conectadas, añadimos algo que hacer: **monedas**. No hay una en todas las salas (solo en `room_01`, `room_03` y `room_04`), se recogen al tocarlas y **no reaparecen** aunque salgas de la sala y vuelvas.

Parece poco, pero obliga a tomar tres decisiones de diseño que se repiten en cualquier juego.

## 1. Datos del nivel vs. estado de la partida

```cpp
struct Coin { float x, y, size = 8.0F; };   // dato del NIVEL
class CoinCollection { /* qué monedas has cogido */ };   // estado de la PARTIDA
```

`Coin` **no sabe si ha sido recogida**. Es la descripción de dónde hay una moneda, y no cambia nunca mientras juegas. Lo que cambia es *qué has recogido*, y eso vive aparte, en `CoinCollection`.

Es la misma distinción que en frontend entre la *configuración* de una pantalla y el *estado* de la aplicación. ¿Por qué importa?

- Si `Coin` tuviera un campo `collected`, para reiniciar la partida tendrías que recorrer el mundo "des-recogiendo" monedas, o recargar los niveles.
- Con la separación, una partida nueva es simplemente una `CoinCollection` vacía. El mundo no se toca.
- El mismo `RoomGraph` podría usarse en dos partidas a la vez.

### ¿Cómo se identifica una moneda?

Por **sala + posición en la lista de monedas de la sala**:

```cpp
class CoinCollection {
public:
    bool isCollected(const RoomId& room, std::size_t coin_index) const;
    bool collect(const RoomId& room, std::size_t coin_index);   // true solo la primera vez
    int count() const;
private:
    std::set<std::pair<RoomId, std::size_t>> collected_;
};
```

Como las salas guardan el estado en la sesión y no dentro de la sala, **sobrevive a cambiar de pantalla** sin hacer nada especial. Un detalle: el índice depende del orden de los objetos en el fichero de Tiled, así que si reordenas las monedas de una sala *en medio de una partida guardada* se descuadrarían. Hoy no guardamos partidas, pero es una limitación a recordar si algún día lo hacemos (la solución sería usar el `id` del objeto de Tiled).

## 2. La regla de recogida

```cpp
std::vector<std::size_t> coinsTouchedBy(const Player& player, const Room& room,
                                        const CoinCollection& collection);
```

Es una función pura: dados el jugador, la sala y lo ya recogido, devuelve qué monedas está tocando. Usa el `AABB::overlaps` de E3, así que solo rozar el borde no cuenta. Se testea sin ventana, con salas y jugadores construidos a mano.

Fíjate en que **no modifica nada**: solo *pregunta*. Quien decide qué hacer con la respuesta es `GameSession`. Separar "consultar" de "actuar" hace que las reglas sean fáciles de probar.

## 3. Eventos: informar sin acoplar

Aquí está la novedad de arquitectura. Cuando recoges una moneda, ¿quién se entera? En un juego pequeño tentaría llamar directamente a "reproducir sonido" o "mostrar efecto" desde la lógica. Pero el dominio no sabe nada de sonido ni de pantalla.

La solución: `GameSession::update` **devuelve una lista de eventos** de lo que ha pasado en ese paso:

```cpp
struct RoomChanged  { RoomId from; RoomId to; };
struct CoinCollected { RoomId room; std::size_t coin_index; int total_collected; };
using GameEvent = std::variant<RoomChanged, CoinCollected>;

std::vector<GameEvent> GameSession::update(const InputState& input);
```

`std::variant` es el equivalente en C++ de una *unión discriminada* de TypeScript (`type GameEvent = RoomChanged | CoinCollected`). Los eventos se llaman en pasado ("ha ocurrido") y son datos puros.

Quien llama decide qué hacer con ellos. Hoy, `main.cpp` los imprime por consola:

```cpp
void report(const application::GameEvent& event) {
    if (const auto* coin = std::get_if<application::CoinCollected>(&event)) {
        std::cout << "[event] coin collected in " << coin->room << ...
    }
}
```

Mañana, el mismo punto podrá reproducir un sonido o lanzar partículas **sin cambiar ni una línea de la lógica del juego**. Esto es lo mismo que un *event bus* o los *domain events* de DDD.

La mayoría de las veces el vector estará vacío, y un `std::vector` vacío no reserva memoria, así que devolverlo en cada paso no cuesta nada.

### Un detalle de orden

En `update`, las monedas se comprueban **después** de procesar el cambio de sala:

```cpp
// Look at the room the player is in NOW (it may have just changed).
const domain::Room& here = currentRoom();
for (const std::size_t index : domain::coinsTouchedBy(player_, here, collected_coins_)) { ... }
```

Si se comprobaran antes, una moneda pegada al borde de la sala nueva (donde apareces) se recogería un paso tarde. Es un caso borde que se arregla simplemente ordenando bien.

## Las monedas en Tiled

Una moneda es un **objeto rectángulo** en una capa de objetos, con `Class = coin`:

```json
{ "id": 1, "class": "coin", "x": 200, "y": 116, "width": 8, "height": 8 }
```

El loader acepta también el campo antiguo `type` (Tiled lo renombró a `class` en la versión 1.9) y usa el ancho del objeto como tamaño. Cualquier otro objeto (por ejemplo un futuro punto de aparición) se ignora, así que podemos ir añadiendo tipos de objetos sin romper nada.

Para colocar una moneda en Tiled: capa de objetos → herramienta *Insert Rectangle* → en sus propiedades, *Class* = `coin`.

## Dibujo y HUD

La capa de presentación pinta las monedas que **aún no están recogidas** (un cuadrado naranja con centro claro) y un contador arriba a la izquierda con la fuente de depuración que incluye SDL3:

```cpp
SDL_RenderDebugText(renderer, 6.0F, 6.0F, "COINS 1/3");
```

Es una fuente de 8×8 píxeles, suficiente para empezar sin cargar fuentes ni imágenes. El total sale de `RoomGraph::totalCoins()`.

## Tests

- **`coin_test`**: una moneda solo se toca al solaparse (no al rozar), las ya recogidas se ignoran, y una moneda recogida en *otra* sala no esconde la de esta.
- **`game_session_test`**: tocar una moneda genera un evento `CoinCollected` con el total correcto; la misma moneda nunca se recoge dos veces; **al salir y volver a la sala sigue recogida**; el cambio de sala emite `RoomChanged`.
- **`tiled_map_loader_test`**: se leen monedas con `class` y con el `type` antiguo, se ignoran otros objetos, y las salas reales tienen 1, 0, 1 y 1 monedas. Otro test comprueba que **ninguna moneda está enterrada dentro de un tile sólido**.

Y, como en E5, comprobé con el buscador de estados desechable que **las tres monedas se pueden tocar de verdad** con la física real, para no colocar ninguna donde no se llegue.

## Cómo probarlo

```bash
cmake -S game -B build && cmake --build build
ctest --test-dir build --output-on-failure
./build/platformer
```

Las monedas están en:

- `room_01`: sobre la plataforma de la derecha (la segunda por altura).
- `room_03`: sobre la plataforma más alta. Llegas desde la escalera de `room_02`.
- `room_04`: sobre la plataforma de la derecha.

Al recoger una, el contador sube y la moneda desaparece. Sal de la sala y vuelve: sigue sin estar. Si arrancas el juego desde un terminal verás también los eventos impresos (`[event] coin collected...`).

## Siguiente: E7

Pulido: sustituir los rectángulos por **sprites** (jugador, moneda y tiles usando el `tiles.png`), y repasar la documentación con un resumen de la arquitectura completa. Dejaremos el juego listo para las siguientes epicas: mando, sonido y enemigos.
