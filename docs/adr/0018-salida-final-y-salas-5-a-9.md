# ADR-0018: Salida final y salas 05 a 09

- Estado: Aceptada (el tamaño del dibujo de la salida, sustituido por ADR-0019)
- Fecha: 2026-11-23

## Contexto
El juego no tenía final: cuatro salas sin meta. La tarea 20 pedía más salas y una forma de ganar.

## Decisión
**Meta** (decidida por el autor): **llegar a una salida**, sin exigir monedas.
- `Exit{x, y, size}` es dato de nivel (clase `exit` de Tiled, símbolo `X` en texto, `Room::exits()`). Al tocarla, `GameSession` marca `won()`, emite `GameWon{coins, total_coins}` y **deja de avanzar** (`update` devuelve vacío).
- **`GameFlow` tiene una pantalla más, `Won`** (ADR-0016): `win()` la abre, con "BACK TO TITLE" y "QUIT" (Esc vuelve al título). `main.cpp` llama a `flow.win()` al ver `GameWon`. La pantalla muestra **"YOU ESCAPED" y las monedas recogidas sobre el total**.
- **Sonido:** `win` (un arpegio ascendente y una nota larga) para `GameWon`.
- **Dibujo:** `sprites/exit.png` (`make_exit.py`): un arco de piedra con la luz del día que pulsa, de 16×16, centrado en la hitbox de 8×8 y sentado en el suelo.
- **`level_check`** exige poder tocar la salida (como monedas, frutas y checkpoints).

**Mundo** (decidido por el autor: 5 o más salas): cinco salas nuevas, generadas por `game/tools/make_rooms_05_09.py` (texto → `level_from_text` → `.tmj`; se edita el script y no los ficheros):
- `room_05` *el pasillo*: foso con pinchos, un escarabajo y un checkpoint; conecta con `room_04` por el este.
- `room_06` *la subida*: plataformas en zigzag hasta el techo (por el norte), un escarabajo en una plataforma, frutas y checkpoint.
- `room_07` *la caverna*: se llega **por un hueco del suelo** (como `room_03`), con pinchos, plataforma colgante con dos estalactitas trampa, un escarabajo; se sale por el oeste.
- `room_08` *la joroba*: una rampa de 5 tiles que sube a una meseta con checkpoint, y pinchos y un escarabajo al otro lado.
- `room_09` *la salida*: foso con pinchos, estalactitas, un escarabajo y un checkpoint justo antes de la salida.
- Total: **13 monedas** y 7 escarabajos; el mundo tiene 9 salas. Los recorridos se invierten entre salas (en las verticales la geometría no es coherente: es un mapa por conexiones, como hasta ahora).

**Lecciones de diseño** (las fijó `level_check`):
- Una conexión norte/sur coloca al jugador al otro extremo **en el mismo `x`**: la sala de destino necesita un hueco en el suelo bajo ese `x`, y la plataforma de salida debe estar **a menos de unos 6 tiles del techo** para que el jugador salga con velocidad de subida suficiente para salir del hueco.
- Una plataforma colgante solo cuenta si hay forma de llegar: la de `room_07` necesita un escalón.

## Alternativas descartadas
- **Exigir todas las monedas o un mínimo para abrir la salida:** ata el final a la exploración; el autor prefirió una meta simple.
- **Una sala final distinta del resto (jefe):** fuera de alcance.
- **Un solo `.tmj` hecho a mano en Tiled:** más lento de iterar que el generador de texto.

## Consecuencias
- `level_check` explora ~16 M estados (el límite es 20 M): **la próxima sala puede obligar a subirlo o a podar la búsqueda** (backlog 13). En Release tarda unos 6 s.
- Los escarabajos y las estalactitas siguen sin modelarse en `level_check` (ADR-0014): la dificultad real de las salas nuevas solo se comprueba jugando.
- No hay tiempo de partida ni mejor marca (requeriría guardado, tarea 19).
