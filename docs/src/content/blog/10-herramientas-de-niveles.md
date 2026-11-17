---
title: "Herramientas de niveles: probar que se puede ganar, y dibujar en texto"
epic: "Extra · Herramientas"
date: 2026-11-17
summary: "Tres programas en C++ que reutilizan la física del juego: un comprobador de alcanzabilidad y dos conversores entre texto y mapas de Tiled."
---

Hasta ahora los niveles los generé con scripts de Python desechables, y las pruebas de "¿se puede llegar?" eran programas sueltos que no quedaban en el repositorio. Este hito lo convierte en **herramientas reales y con tests**, escritas en C++ para **reutilizar exactamente la misma física del juego**.

## Por qué C++ y no Python

El comprobador necesita saber qué puede hacer el jugador: cuánto salta, cómo choca, cuándo cambia de sala. Eso ya está escrito en el dominio (`stepPlayer`, `confineToMap`, `exitedThrough`, `enterRoom`). En Python habría que **copiar** esas reglas, y las copias se desincronizan: cambias la gravedad en el juego y el comprobador sigue creyendo en la vieja.

En C++, la herramienta enlaza con la **misma librería `domain`** que el juego. Si cambias la física, el comprobador cambia con ella, sin tocar nada.

Esto es posible gracias a la arquitectura de E0: el dominio no depende de SDL, así que cualquier programa (el juego, los tests, una herramienta) puede usarlo.

## Estructura

```
game/tools/
  level_tools/             librería: la lógica (con tests)
    reachability.*           análisis de alcanzabilidad
    text_level.*             formato de texto <-> mapa
  level_check.cpp          programa 1: comprobar niveles
  level_from_text.cpp      programa 2: texto -> .tmj
  level_to_text.cpp        programa 3: .tmj -> texto
game/levels/example.txt    un nivel de ejemplo en texto
```

Como en el resto del proyecto, **la lógica va en una librería** y los programas (`main`) son finos: leen argumentos, llaman a la librería e imprimen. Así todo lo importante se puede testear.

## 1. `level_check`: demostrar que se puede ganar

```bash
./build/level_check game/assets/tiled room_01
```

```
Rooms:
  room_01  reachable, can stand
  room_02  reachable, can stand
  ...
Coins:
  room_01 #0 at (200, 116)  OK
  room_03 #0 at (168, 100)  OK
  ...
Result: OK
```

Responde a dos preguntas: **¿se puede llegar a todas las salas?** y **¿se puede tocar cada moneda?**

### Cómo funciona

El jugador solo puede hacer 6 cosas en cada paso: ir a la izquierda, a la derecha o quedarse quieto, y saltar o no. El comprobador hace una **búsqueda en anchura** (BFS): parte del punto de aparición, prueba las 6 posibilidades, y de cada resultado vuelve a probar las 6, sin repetir estados ya vistos.

```cpp
for (const domain::InputState& input : inputs) {
    State next = current;
    domain::stepPlayer(next.player, input, room.map());
    domain::confineToMap(next.player, room.map(), domain::openEdgesOf(room));
    if (const auto side = domain::exitedThrough(next.player, room.map())) {
        // ... cambiar de sala
    }
    if (seen.insert(keyOf(next)).second) queue.push_back(next);
}
```

Es el mismo bucle que `GameSession::update`, pero probando **todas** las entradas posibles en vez de las que pulsa un jugador.

### ¿Por qué termina?

Parece que el número de estados es infinito (la posición es un número decimal). Pero redondeamos la posición (0,5 px) y la velocidad (0,1) para identificar un estado. Eso lo hace **finito**. En los niveles actuales son unos 377.000 estados y tarda poco más de un segundo.

### Es una prueba, no una estimación

La física es **determinista**: la misma entrada da siempre el mismo resultado. Por eso, si la búsqueda dice "esta moneda es alcanzable", existe una secuencia de teclas que la consigue; y si dice "no", no hay ninguna (salvo el redondeo, que podría perder algún caso extremo de un píxel).

### Un detalle: el punto de aparición

El juego hace aparecer al jugador **en el aire** (y = 60) y lo deja caer. Eso significa que una plataforma justo debajo del punto de aparición se alcanza "gratis", cayendo sobre ella. El comprobador usa por defecto el mismo punto que el juego (puedes cambiarlo con `level_check carpeta sala x y`), pero los tests usan la posición en el suelo para medir lo que se consigue **saltando**. Hay un test que documenta esta diferencia.

### En CTest

Está conectado como un test más:

```cmake
add_test(NAME shipped_levels_reachable
         COMMAND level_check ${CMAKE_CURRENT_SOURCE_DIR}/assets/tiled room_01)
```

Así `ctest` falla si alguien edita un nivel y deja una moneda inalcanzable. Los códigos de salida son `0` (todo bien), `1` (hay algo inalcanzable) y `2` (entrada incorrecta).

## 2. Niveles dibujados en texto

Un nivel se ve como lo que es:

```text
# comentario
east: room_02
---
........................................
...............C........................
..........BBBB..........................
########################################
```

| Símbolo | Significado |
|---|---|
| `.` | vacío |
| `#` | suelo (tile 1) |
| `B` | ladrillo (tile 2) |
| `3`, `4` | tiles 3 y 4 del tileset (sólidos, se dibujan como ladrillo); existen para que mapas de Tiled que los usen se conviertan sin pérdida |
| `C` | moneda, sobre un tile vacío |

Encima de la línea `---` va la cabecera: `north`, `south`, `east`, `west` con el id de la sala vecina, y `tile_size` opcional. Las líneas que empiezan por `#` en la cabecera son comentarios (en la cuadrícula, `#` es suelo, así que ahí no hay comentarios).

```bash
./build/level_from_text game/levels/example.txt game/assets/tiled/room_05.tmj
```

### Errores con número de línea

Si te equivocas, el mensaje dice dónde:

```
error in broken.txt: line 3: unknown character 'x' (use . # B C)
```

Los errores comprobados: separador `---` ausente, filas de distinto largo, caracteres desconocidos, claves de cabecera inválidas y cuadrícula vacía. Cada uno con su test.

### Un solo formato intermedio

`TextLevel` es una estructura simple (tiles, monedas, vecinos) que sirve de puente. Se puede convertir a una `Room` del dominio (`toRoom`) y a un mapa de Tiled (`toTiledJson`). Eso permite, por ejemplo, **analizar un nivel en texto sin escribir ningún fichero**: el test del ejemplo lo hace así.

## 3. `level_to_text`: el camino de vuelta

```bash
./build/level_to_text game/assets/tiled/room_02.tmj room_02.txt
```

Dibuja un mapa existente como texto. Sirve para **migrar** los niveles actuales al formato de texto, o simplemente para verlos de un vistazo en la terminal. Hay dos límites, y los tests los comprueban:

- Solo entiende los tiles 1 y 2 (si encuentra otro, da un error claro).
- Las monedas se **ajustan a la cuadrícula** del tile más cercano. Las de las salas actuales estaban a mitad de tile, así que al pasar por el texto se mueven unos píxeles.

Hay un test que pasa las 4 salas reales por `tmj → texto → tmj` y comprueba que los tiles, vecinos y número de monedas son idénticos.

## ¿Texto o Tiled?

No son excluyentes, pero conviene tener claro **cuál es la fuente de verdad** de cada nivel, para no editar el mismo mapa por dos vías y que una pise a la otra:

| | Texto | Tiled |
|---|---|---|
| Ideal para | esbozar rápido, versionar en Git | diseñar con ratón, ver el resultado |
| Se lee en un diff | sí | difícilmente (JSON largo) |
| Requiere programa | no | sí |

Una forma sensata de combinarlos: esbozar en texto, convertir, y **a partir de ahí** pulir en Tiled (y dejar de usar el texto para esa sala). En el siguiente paso veremos Tiled en detalle.

## Tests

`level_tools_test` (36 casos en total en el proyecto):

- **Alcanzabilidad**: moneda en el suelo, moneda a altura de salto, moneda demasiado alta, escalera de plataformas, plataforma inalcanzable, salas enlazadas y aisladas, muro demasiado alto, sala de inicio desconocida y límite de estados.
- **Texto**: lectura de tiles/monedas/vecinos, finales de línea de Windows, errores con línea, y las dos conversiones de ida y vuelta.
- **Niveles reales**: las 4 salas pasan `tmj → texto → tmj` sin cambios, y `example.txt` es válido y tiene sus 3 monedas alcanzables.

## Lo que aprendimos de un fallo

Al escribir los tests, uno falló: una plataforma a 80 px de altura, que no se puede saltar, aparecía como alcanzable. No era un fallo de la física: era la aparición en el aire de la que hablábamos antes (el jugador cae encima). Lo bueno de una herramienta que *prueba* en vez de *estimar* es que te muestra estos supuestos escondidos; y lo bueno de escribir tests es que te obligan a mirar el porqué.

También el compilador avisó de una **referencia colgante** en mi código: `neighbor()` devuelve el valor por copia (`std::optional`), y yo guardaba una referencia al interior de ese temporal. La solución fue copiar el valor. Es un clásico de C++ que en lenguajes con recolector de basura no existe.

## Siguiente

Ahora que podemos comprobar niveles, toca la parte visual: **abrir y editar los mapas en Tiled**.
