---
title: Herramientas y comprobador
section: niveles
order: 2
summary: Cómo level_check demuestra que un nivel se puede completar.
---

| Herramienta | Qué hace |
|---|---|
| `level_check` | comprueba que todo es alcanzable |
| `level_from_text` | texto → mapa de Tiled |
| `level_to_text` | mapa de Tiled → texto |

```bash
./build/level_check game/assets/tiled room_01
./build/level_from_text game/levels/example.txt example.tmj
./build/level_to_text game/assets/tiled/room_02.tmj room_02.txt
```

## Cómo lo demuestra

Prueba **todo lo que se puede hacer**. Desde el punto de partida, en cada paso intenta las 6 combinaciones
(izquierda, derecha o nada × saltar o no) usando la **física real**, y apunta las situaciones nuevas. Es una
búsqueda en anchura, como explorar un laberinto:

<!-- src: game/tools/level_tools/reachability.cpp -->
```cpp
        for (const domain::InputState& input : inputs) {
            State next = current;
            domain::stepPlayer(next.player, input, room.map());
```

Un estado ya visto no se repite. Para que el número sea finito, posición y velocidad se **redondean**:

<!-- src: game/tools/level_tools/reachability.cpp -->
```cpp
    const auto x = static_cast<std::uint64_t>(std::llround(state.player.x * 2.0F) + 8192);
    const auto y = static_cast<std::uint64_t>(std::llround(state.player.y * 10.0F) + 8192);
```

Las rutas que tocan pinchos se descartan.

## Hacerlo rápido

Explorar un juego entero son millones de situaciones, así que importa cómo se guardan. En vez del conjunto de la
biblioteca estándar (`std::unordered_set`, que reserva memoria para cada elemento, unos 40 bytes), se usa
`StateSet`: **una sola lista** de números en la que, si el hueco de una clave está ocupado, se usa el siguiente.

<!-- src: game/tools/level_tools/state_set.h -->
```cpp
        for (std::size_t at = static_cast<std::size_t>(mix(key)) & mask;; at = (at + 1) & mask) {
            if (slots[at] == key) {
                return false;
            }
            if (slots[at] == kEmpty) {
                slots[at] = key;
                return true;
            }
        }
```

`mix` revuelve los bits para repartir claves parecidas, y `& mask` es el resto de dividir por el tamaño (que
siempre es potencia de dos). Es una **tabla hash de direccionamiento abierto**: ocupa menos y, al estar todo
junto en memoria, el procesador va mucho más rápido.

Además, **se detiene en cuanto lo ha encontrado todo**: seguir no cambiaría la respuesta. Si algo no aparece,
explora hasta el final antes de decir que no se puede. Con `--full` explora todo siempre:

```bash
./build/level_check --full game/assets/tiled room_01
```

Con las salas actuales tarda unos 7 segundos y usa unos 550 MB (antes, 15 segundos y 1,2 GB).

## Lo que NO comprueba

> Un "OK" no significa que el nivel sea bueno ni fácil.

- **Escarabajos** y **estalactitas trampa**: los ignora.
- **Agacharse y deslizarse**: nunca pulsa abajo.
- **El orden**: comprueba que cada cosa se alcanza desde el inicio, no que puedas ir de una a otra. Tras cambiar
  salas, lánzalo también desde la final:

```bash
./build/level_check game/assets/tiled room_09 20 190
```

- Tiene un límite de 60 millones de estados y las salas actuales necesitan unos 24: si se supera, la
  comprobación se da por **incompleta** (`INCOMPLETE`).

El proyecto se compila optimizado (*Release*) por defecto; sin optimizar, el comprobador es unas diez veces más
lento.

Decisiones: [ADR-0003](../decisiones/0003-comprobador-de-alcanzabilidad/) y
[ADR-0028](../decisiones/0028-comprobador-mas-rapido/).
