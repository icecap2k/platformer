---
title: "Editar los mapas con Tiled"
epic: "Extra · Herramientas"
date: 2026-11-17
summary: "Cómo abrir las salas del juego en Tiled, qué reglas hay que respetar (capas, CSV, propiedades, monedas) y cómo comprobar que el nivel sigue siendo ganable."
---

En el post anterior vimos cómo esbozar un nivel en texto y convertirlo en un mapa. Ahora toca la otra mitad: **pulir los mapas con ratón**, en [Tiled](https://www.mapeditor.org/). El juego no sabe nada de Tiled como programa: solo lee los ficheros `.tmj` que produce. Mientras respetes unas pocas reglas, cualquier cosa que dibujes funciona.

## Instalar y abrir

```bash
brew install --cask tiled
```

Abre cualquiera de las salas de `game/assets/tiled/` (`room_01.tmj`…). Los mapas usan el tileset `tiles.png`, que va incrustado en cada `.tmj`, así que Tiled lo encuentra solo.

## Lo que el juego espera de un mapa

El cargador (`infrastructure/assets/tiled_map_loader.cpp`) es deliberadamente simple. Estas son sus reglas, y por qué:

| Regla | Por qué |
|---|---|
| Tiles **cuadrados** y mapa **no infinito**; usa **8×8** y **40×30** | El cargador solo exige lo primero. El 8×8 y el 40×30 son la convención del juego (resolución lógica); otros tamaños no se han probado. |
| Se usa **la primera capa de tiles** | Una sola capa decide qué es sólido. Si añades otra encima, se ignora. |
| La capa debe estar en **CSV** | El cargador no descomprime ni decodifica base64. Tiled lo guarda así si en *Propiedades del mapa → Codificación de capas de tiles* eliges `CSV`. |
| Cualquier tile **distinto de 0 es sólido** | `isSolid` es el único sitio que decide esto. El 1 es suelo y el 2 ladrillo; se dibujan distinto, pero chocan igual. |
| Las monedas son **objetos de clase `coin`**; usa **rectángulos** | Van en una capa de objetos, no de tiles. El cargador solo mira la clase, pero la posición se interpreta como esquina superior izquierda: un objeto-tile (cuyo origen está abajo) quedaría desplazado. |
| Las salas vecinas son **propiedades del mapa** de tipo texto: `north`, `south`, `east`, `west` | El valor es el nombre del fichero sin extensión (`room_02`). Sin propiedad, ese borde es un muro. |

Dos detalles de Tiled que despistan:

- Las propiedades del **mapa** están en *Mapa → Propiedades del mapa*, no en las de una capa. Un `east` puesto en la capa no se lee.
- Desde Tiled 1.9 el campo se llama *Clase* (`class`) y antes *Tipo* (`type`). El cargador acepta los dos.

## Receta: añadir una sala nueva

1. **Archivo → Nuevo mapa**: ortogonal, 40×30 tiles, tiles de 8×8, formato CSV. Guárdalo como `room_05.tmj` en `game/assets/tiled/`.
2. Añade el tileset `tiles.png` (el de los mapas existentes está incrustado en el propio `.tmj`; haz lo mismo con *Incrustar en el mapa*).
3. Dibuja el suelo y las plataformas en la capa de tiles. Deja libre el borde por el que se pasa a la sala vecina.
4. Crea una capa de objetos y, con la herramienta *Insertar rectángulo*, coloca cada moneda. En el panel de propiedades pon **Clase = `coin`** y tamaño 8×8.
5. En *Propiedades del mapa* añade, por ejemplo, `west = room_04` (tipo *string*).
6. **Vuelve atrás**: abre `room_04` y añade `east = room_05`. Los enlaces tienen que ser recíprocos, y hay un test que lo comprueba.
7. Añade `"room_05"` a la lista de `loadWorld()` en `main.cpp` y a las listas de salas de los tests de niveles reales (`tiled_map_loader_test`, `game_session_test`, `level_tools_test`), incluidos los recuentos de monedas.

## Comprobar que se puede ganar

Dibujar a ojo engaña: una plataforma que parece alcanzable puede estar 8 píxeles demasiado alta. Después de editar, pasa el comprobador del post anterior:

```bash
cmake --build build
./build/level_check game/assets/tiled room_01
```

Usa la misma física que el juego, así que si dice `OK`, hay un camino real. Si una moneda sale como `CANNOT BE TOUCHED`, baja la plataforma o añade un escalón intermedio y vuelve a probar. Es un ciclo rápido: editar en Tiled, guardar, ejecutar el comprobador.

## ¿Y qué pasa con el texto?

Como adelantábamos: elige **una fuente de verdad por sala**. Una forma cómoda de trabajar es esbozar en texto, convertir con `level_from_text` y, desde ese momento, editar solo en Tiled. Si luego quieres ver el mapa como texto (para un diff legible), `level_to_text` hace el camino inverso sin tocar el `.tmj`.

## Errores típicos

| Síntoma | Causa probable |
|---|---|
| `Tile layer data must be uncompressed` | La capa no está en CSV (comprimida o en base64): cambia la codificación a CSV y guarda otra vez. |
| `Tiled map has no tile layer` | Solo hay capas de objetos. |
| Cruzas el borde y no pasa nada | Falta la propiedad (`east`, `west`…), o está en la capa y no en el mapa. |
| La moneda no aparece o sale desplazada | La clase no es exactamente `coin`, o el objeto no es un rectángulo (un objeto-tile tiene el origen abajo). |
| Los tests de enlaces fallan | La sala nueva apunta a otra que no apunta de vuelta. |

## Siguiente

Con el mapa en orden, volvemos a la jugabilidad: **un salto más fino**, con altura variable y *coyote time*.
