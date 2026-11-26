---
title: Carga de mapas
section: infraestructura
order: 2
summary: Cómo un fichero de Tiled se convierte en una Room del juego.
---

Las salas son mapas de [Tiled](https://www.mapeditor.org/) en formato `.tmj`, que es **JSON**. El cargador
(`tiled_map_loader.cpp`) lo traduce a objetos del dominio. No usa SDL, así que se prueba como el resto.

## De JSON a `Room`

<!-- src: game/src/infrastructure/assets/tiled_map_loader.cpp -->
```cpp
        const json map = json::parse(json_text);
        return domain::Room(room_id, readTiles(map), readNeighbors(map), readCoins(map),
```

Hay una función pequeña por cada cosa. Todas siguen el mismo patrón: recorrer las capas de objetos y quedarse
con los de su **clase**:

<!-- src: game/src/infrastructure/assets/tiled_map_loader.cpp -->
```cpp
            const std::string kind = object.value("class", object.value("type", std::string()));
            if (kind != "coin") {
                continue;
            }
```

(Tiled antiguo llama `type` a lo que el nuevo llama `class`; se aceptan los dos.)

## Fallar pronto y claro

Un mapa mal hecho detiene el juego al arrancar con un mensaje que dice **qué** falla:

<!-- src: game/src/infrastructure/assets/tiled_map_loader.cpp -->
```cpp
            const auto found = kColours.find(colour);
            if (found == kColours.end()) {
                throw std::invalid_argument("an 'orb' object needs a 'colour' property: red, green or yellow");
            }
```

Es la idea de **validar en la frontera**: lo que entra al dominio ya es correcto.

El detalle de qué puede contener un mapa está en [Formato de los niveles](../formato-de-niveles/).
