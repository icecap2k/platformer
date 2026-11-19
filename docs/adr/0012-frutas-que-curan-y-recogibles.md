# ADR-0012: Frutas que curan y recogibles

- Estado: Aceptada
- Fecha: 2026-11-19

## Contexto
Hay corazones y daño (ADR-0005), pero nada los recupera. El backlog pedía **frutas que curan** y avisaba de un riesgo: generalizar los coleccionables **antes** de duplicar el código de las monedas. Unos 30 ficheros usan `Coin`, así que renombrarlo todo habría sido mucho ruido para un solo tipo nuevo.

## Decisión
**Comportamiento** (decidido por el autor):
- La fruta cura **1 corazón** (`kFruitHealing`), nunca por encima de `kMaxHealth`.
- Con la salud al máximo **se come igual y se desperdicia** (no queda para después). El evento lo dice: `hearts_gained == 0`.
- Es una **baya luminosa de cueva** (rosa-naranja, con un brillo que pulsa, a juego con setas y cristales y distinta de la moneda dorada y del corazón rojo).
- El reinicio total (ADR-0005) las devuelve todas a su sitio.

**Datos y dominio:**
- `Fruit{x, y, size}` es **dato de nivel**, igual que `Coin`, y `Room::fruits()` es una lista aparte. Si más adelante aparece un tercer tipo, ese será el momento de introducir un `Collectible` con tipo; hoy se prefirió no tocar el resto de ficheros.
- **Lo que sí se generalizó** para no duplicar: la colección de recogidos pasa a llamarse `PickupCollection` (identifica un recogible por sala e índice en la lista de su tipo; `CoinCollection` sigue existiendo como alias) y la detección de contacto es una plantilla, `pickupsTouchedBy`, que usan `coinsTouchedBy` y `fruitsTouchedBy`. Cada tipo tiene **su propia colección**.
- `domain::heal(player, cantidad)` devuelve los corazones realmente ganados (`rules/healing.h`).
- `GameSession`: tras las monedas y antes de los peligros, come las frutas que toca, cura y emite `FruitEaten{room, fruit_index, health_after, hearts_gained}`. `eatenFruits()` expone la colección.

**Niveles y herramientas:**
- Objetos de Tiled de clase `fruit` (la clase está añadida a `platformer.tiled-project`); símbolo `F` en el formato de texto, con el mismo tratamiento que `C` (se ajusta al tile; no puede estar sobre un sólido).
- **`level_check` las trata como las monedas:** una fruta que no se pueda tocar hace fallar el nivel (informe y `allOk`).
- Colocadas: `room_02` (sobre el suelo, justo pasados los pinchos) y `room_04` (sobre la plataforma de la izquierda). Un test fija su número y que ninguna quede enterrada ni flotando.

**Dibujo:** cuatro fotogramas de **16×16** en filas nuevas de `sprites.png` (y = 34), que genera `make_player_sprites.py`: la baya (8×8) en el centro de un **halo cálido semitransparente que respira** (crece y se ilumina, luego se encoge) con destellos en cruz que cambian de sitio. `fruitFrame(tiempo)` los cicla cada 160 ms y `fruitBob(tiempo)` la balancea ±1 px cada 0,3 s. El renderer centra el fotograma sobre la fruta, así que el halo desborda su hitbox. La primera versión (8×8 con unos píxeles de brillo) era tan sutil que el autor no la notó: **un efecto de brillo sobre un fondo oscuro necesita transparencia y tamaño**, no solo color.

## Alternativas descartadas
- **Que no se pueda comer con la salud llena** (queda para después): se descartó a favor de la versión simple, aunque castiga el descuido.
- **Curar toda la salud:** habría obligado a poner muy pocas frutas.
- **Renombrar `Coin` a `Collectible` con tipo ahora:** mucho cambio mecánico en unos 30 ficheros para un solo tipo nuevo.

## Consecuencias
- Un coleccionable nuevo requiere: su dato, su lista en `Room`, su lector de Tiled, su símbolo de texto, su entrada en `level_check` y su dibujo. Con tres tipos compensará el `Collectible` genérico.
- Frutas y monedas son independientes: coger una no cuenta como la otra, aunque estén en el mismo sitio.
- Los eventos `FruitEaten` están listos para el sonido.
