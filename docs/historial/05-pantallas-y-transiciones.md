---
title: "Pantallas conectadas: el mundo como grafo de salas"
epic: "E5 · Pantallas y transiciones"
date: 2026-11-17
summary: "Salir por cualquiera de los 4 bordes carga la sala vecina. Modelamos el mundo como un grafo y las reglas de salida como dominio puro."
---

El objetivo de este hito: cuando el personaje llega al final de la pantalla por **cualquiera de los cuatro lados**, carga la siguiente y aparece por el lado contrario. Para que sea fácil de ampliar, el mundo se modela como un **grafo**: nodos (salas) y aristas (qué hay detrás de cada borde).

## El modelo

```
Side         enum: North, South, East, West      (+ opposite(side))
Room         id + TileMap + vecinos por lado
RoomGraph    todas las salas por id, con enlaces validados
```

```cpp
class Room {
public:
    Room(RoomId id, TileMap map, std::map<Side, RoomId> neighbors = {});
    std::optional<RoomId> neighbor(Side side) const;
};
```

Un lado **sin vecino es una pared**. No hace falta declararlo: la ausencia de enlace ya lo dice. Así la regla es una sola: si hay vecino, se puede cruzar; si no, es muro.

`RoomGraph` valida en su constructor (como `TileMap` hacía con sus tiles): lanza `std::invalid_argument` si hay ids duplicados o si una sala apunta a otra que no existe. Un grafo que se ha construido es siempre coherente, así que el resto del código no tiene que comprobar si el destino existe.

### Las salas se definen en Tiled

Cada `.tmj` lleva **propiedades personalizadas del mapa** (en Tiled: *Map → Map Properties*):

| Propiedad | Tipo | Valor |
|---|---|---|
| `east` | string | `room_02` |

Solo se leen `north`, `south`, `east` y `west`; cualquier otra propiedad se ignora. El id de la sala es el nombre del fichero, así que `room_02.tmj` es la sala `room_02`. Los cuatro niveles de ejemplo están conectados así:

```
            room_03
               │ (norte/sur)
room_01 ─── room_02 ─── room_04
        (este/oeste)
```

## ¿Dónde viven las reglas?

Esto es lo más interesante de diseño. En E4 la física recortaba al jugador en los bordes del mapa. Ahora **lo hemos quitado de la física**: que un borde sea muro o puerta es una regla del *grafo de salas*, no de la física de movimiento. `stepPlayer` solo conoce tiles.

Las reglas están en `domain/rules/room_transition`:

```cpp
OpenEdges openEdgesOf(const Room& room);                       // qué bordes son puertas
void confineToMap(Player&, const TileMap&, const OpenEdges&);  // los cerrados actúan de pared
std::optional<Side> exitedThrough(const Player&, const TileMap&);  // ¿qué borde cruzó?
void enterRoom(Player&, Side exited_through, const TileMap& new_map);  // reaparecer al otro lado
```

Y `GameSession::update` las encadena, en este orden:

```cpp
domain::stepPlayer(player_, input, room.map());                      // 1. física
domain::confineToMap(player_, room.map(), domain::openEdgesOf(room)); // 2. cerrar muros
if (const auto side = domain::exitedThrough(player_, room.map())) {  // 3. ¿salió?
    current_room_ = *room.neighbor(*side);
    domain::enterRoom(player_, *side, currentRoom().map());
}
```

El orden importa: como el paso 2 convierte los bordes cerrados en paredes, en el paso 3 solo puede "salir" por un borde abierto, y por tanto siempre hay vecino. Por eso el `*room.neighbor(*side)` es seguro sin comprobaciones extra.

### Cuándo cambia la sala: el centro

`exitedThrough` usa el **centro** del jugador, no todo su cuerpo. El cambio ocurre cuando el personaje está a medio camino, lo que se siente natural. Si esperáramos a que saliera entero, vería un parpadeo de "nada" al borde; si cambiáramos al tocar, el cambio sería prematuro.

### Dónde reaparece

`enterRoom` coloca al jugador **justo dentro del borde opuesto** y conserva lo demás:

| Salió por | Aparece | Se conserva |
|---|---|---|
| Este | borde izquierdo (`x = 0`) | altura `y` |
| Oeste | borde derecho | altura `y` |
| Sur | borde superior (`y = 0`) | `x` |
| Norte | borde inferior | `x` |

También se conserva la **velocidad**: si sales volando hacia arriba, sigues subiendo en la sala de arriba; si caes por un agujero, sigues cayendo. Es lo que hace que las transiciones verticales se sientan continuas.

Un test comprueba que, justo después de entrar, el jugador **no** está ya fuera del mapa (si no, rebotaría entre dos salas para siempre):

```cpp
SUBCASE("a freshly entered player is not immediately outside again") {
    for (Side side : {Side::North, Side::South, Side::East, Side::West}) {
        Player player = playerAt(100.0F, 100.0F);
        enterRoom(player, side, map);
        CHECK_FALSE(exitedThrough(player, map).has_value());
    }
}
```

## Diseñar niveles que se pueden recorrer

Los niveles tienen que ser físicamente alcanzables. Para `room_02` → `room_03` hay una escalera de plataformas que sube hasta el borde superior (cada escalón a 40 px de altura, dentro de los 62 px que sube un salto), y en `room_03` el suelo tiene un hueco justo por donde apareces.

Para no fiarme de mi intuición, escribí un programa desechable que explora **todos los estados posibles** (posición, velocidad y teclas) con la física real y comprueba qué salas se pueden alcanzar. Resultado: las 4 salas son alcanzables y se puede aterrizar en todas. Es una técnica muy útil: la física es determinista, así que puedes *probar* que un nivel es superable en vez de jugarlo.

## Tests

- **`room_transition_test`**: paredes en los bordes cerrados, cruce por el centro, reaparición en cada lado, validaciones del grafo.
- **`game_session_test`**: un mundo en forma de cruz con 5 salas donde se cruza por los 4 lados; no se puede salir por un lado sin vecino; ida y vuelta entre dos salas.
- **`tiled_map_loader_test`**: los 4 niveles reales cargan, forman un grafo válido, **todos los enlaces son recíprocos** (si A tiene a B al este, B tiene a A al oeste) y se usan las 4 direcciones.

## Cómo probarlo

```bash
cmake -S game -B build && cmake --build build
ctest --test-dir build --output-on-failure
./build/platformer
```

Recorrido sugerido:

1. En `room_01`, camina hacia la derecha: pasas a `room_02` y puedes volver hacia la izquierda.
2. En `room_02`, sube la escalera de plataformas hacia la derecha hasta la más alta y salta por el borde superior: llegas a `room_03` desde abajo, a través del hueco del suelo.
3. En `room_03`, sal por el hueco del suelo cayendo: vuelves a `room_02` desde arriba.
4. En `room_02`, camina hacia la derecha para llegar a `room_04`, que es un callejón sin salida.

## Siguiente: E6

La **moneda**: un objeto coleccionable que vive en la capa de objetos de Tiled, solo en algunas salas, que se recoge al solaparse con el jugador y no reaparece. Con ella introduciremos los **eventos** de la capa de aplicación y el estado persistente entre salas.
