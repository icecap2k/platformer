# ADR-0027: Las salas se editan en Tiled

- Estado: Aceptada
- Fecha: 2026-11-26

## Contexto
El autor rediseñó las nueve salas en Tiled, colocando objetos también entre casillas. Se había pensado dejar un fichero de texto por sala como original, pero el texto no puede representar esos objetos y regenerar los mapas habría borrado su trabajo. Sustituye la parte de ADR-0018 que generaba las salas 05-09 con un script.

## Decisión
- `game/assets/tiled/room_NN.tmj` es **el original** de cada sala y se edita en Tiled.
- Se eliminan `make_rooms_05_09.py` y los textos de las salas. `level_from_text` queda para bocetos (`game/levels/example.txt`).
- Las pruebas sobre las salas reales ya **no fijan el diseño** (cuántas monedas, dónde está cada cosa): comprueban reglas que, si se rompen, estropean el juego: enlaces recíprocos, tres orbes de colores distintos, una salida, escarabajos que patrullan y nada fuera de la pantalla ni enterrado.
- `level_check` sube su límite a **40 millones** de estados (las salas nuevas necesitan unos 24).

## Alternativas descartadas
- **Texto como original:** habría obligado a ajustar los objetos a la cuadrícula.
- **Mantener las pruebas de diseño:** cada cambio de una sala las rompería.

## Consecuencias
- `level_check` usa alrededor de 1,2 GB de memoria; si las salas crecen, habrá que reducir la búsqueda (backlog 13).
- Ya no se comprueba que los pinchos estén en zanjas ni que las frutas floten: es decisión de quien diseña.
