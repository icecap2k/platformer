---
title: "Salud, daño con retroceso, pinchos y una trampa de estalactita"
epic: "E8 · Peligros"
date: 2026-11-19
summary: "El jugador tiene 3 corazones, un golpe lo lanza hacia atrás con un mini salto y le da un momento de invulnerabilidad. Pinchos y estalactitas que caen son los primeros peligros, y el comprobador de niveles aprende a evitarlos."
---

Hasta ahora nada podía hacerle daño al jugador. Esta épica añade la base del riesgo: **salud, daño, retroceso e invulnerabilidad**, y los dos primeros peligros: **pinchos** y una **estalactita trampa**. Todo se diseñó para que enemigos y frutas que curan encajen después sin cambiar el modelo.

## Un solo camino para hacer daño

Cualquier cosa que haga daño (pinchos hoy, enemigos mañana) pasa por la misma función del dominio:

```cpp
void applyDamage(Player& player, int amount, int direction);
```

Quita corazones (nunca por debajo de 0), activa la **invulnerabilidad** (1,5 s), pone al jugador en **retroceso** y le da un **mini salto**. Como es un único sitio, todos los golpes se sienten igual y ajustar la sensación es cambiar tres constantes.

| Constante | Valor | Efecto |
|---|---|---|
| `kHurtSteps` | 14 pasos | cuánto dura el retroceso (unos 8 px en total) |
| `kKnockbackSpeed` | 0,6 px/paso | velocidad hacia atrás (la primera versión, con 1,2, te lanzaba unos 17 px y se iba demasiado lejos) |
| `kHurtHopSpeed` | 1,7 | el saltito: unos 7 px, frente a los 44 del salto normal (la primera versión, con 2,4, saltaba 14 y se veía demasiado) |
| `kInvulnerableSteps` | 90 pasos | 1,5 s sin poder recibir otro golpe |

La **dirección** del retroceso sale de comparar el centro del jugador con el del peligro: siempre lo aleja. Si están exactamente alineados, va contra el sentido en que se movía.

## Dónde vive cada cosa

La lección de las monedas se repite: **datos del nivel frente a estado de la partida.**

- `Hazard` (pinchos) y `FallingStalactite` son **datos del nivel**: nunca cambian. La hitbox de los pinchos es el rectángulo que dibujes en Tiled, así que cada nivel decide cuánto perdona.
- La **salud** y las dos cuentas atrás (`invulnerable_steps`, `hurt_steps`) están en `Player`.
- El estado de cada trampa (en reposo, temblando, cayendo, rota) está en `StalactiteTraps`, dentro de la sesión, identificado por sala e índice como las monedas.
- `GameSession` junta todo y **avisa con eventos**: `PlayerHurt`, `StalactiteFell` y `GameRestarted`. Quién reacciona (hoy un `cout`, mañana un sonido) no es asunto del juego.

Al llegar a 0 corazones, `GameSession::restart()` lo reinicia **todo**: primera sala, 3 corazones, 0 monedas y las trampas otra vez colgando.

## El jugador, mientras le golpean

`stepPlayer` ignora el input mientras dura el retroceso: el golpe manda, no el jugador. Dos detalles que dieron guerra:

- El corte de salto variable (soltar el botón recorta el salto) **no** debe aplicarse al saltito del golpe, o sería aún más pequeño.
- La animación calcula hacia dónde mira el sprite con la velocidad horizontal; durante el retroceso esa velocidad es la contraria, y el personaje se **giraba de espaldas**. Ahora, mientras está herido, mantiene la dirección en que miraba.

Presentación: una pose nueva (`Hurt`), parpadeo mientras es invulnerable (3 pasos visible, 3 oculto, lógica pura y con tests) y tres corazones arriba a la derecha.

## Pinchos

Son objetos de Tiled de clase `spikes` (igual que las monedas son de clase `coin`). El renderer repite un sprite de 8×8 a lo largo del rectángulo: un diente triangular por tile, de acero claro, de modo que varios juntos forman una sierra. El sprite se dibuja **1 px más abajo** que el borde del rectángulo: su última fila es una línea oscura que se funde con el contorno del suelo, y así los dientes parecen clavados en él. (La primera versión eran agujas finas sobre una tira y no se distinguían bien.) En el formato de texto son `^`, y su hitbox deja libre la parte alta del tile para que las puntas perdonen un poco: el dibujo sobresale 2 px por encima de la zona que daña.

### El comprobador de niveles tuvo que enterarse

`level_check` demuestra que cada sala y moneda se puede alcanzar. Si ignorara los pinchos, "demostraría" caminos que los atraviesan. Ahora **descarta los estados en los que el jugador toca un pincho**. Hay tests: una moneda sobre pinchos no se puede tocar, unos pinchos bajos se saltan, y una pared de peligros más alta que cualquier salto bloquea el paso a la sala siguiente.

Las estalactitas, al ser dinámicas, **no se modelan** en la búsqueda (se pueden esquivar). Es una limitación consciente y está anotada.

## La estalactita trampa

Una máquina de estados sencilla:

```
Idle ──(jugador debajo)──► Shaking ──(30 pasos)──► Falling ──(suelo o jugador)──► Gone
```

- **Aviso:** tiembla medio segundo antes de soltarse, para que el jugador tenga tiempo de apartarse. Una vez que empieza a temblar, caerá aunque te vayas (si no, sería trivial).
- **Debajo:** cuenta que el jugador esté bajo ella (con 12 px de margen a los lados) **sin nada sólido entremedio**, para que una repisa te proteja.
- **Cae** con aceleración, siempre a menos de un tile por paso (la misma regla anti-atravesado que la física del jugador), y se rompe al tocar suelo o al golpear al jugador.
- Se ve distinta de las estalactitas de adorno: es más pálida y tiene grietas, para que se lea como "suelta".

En el formato de texto es una `V` bajo un techo.

## Lo que se rompió por el camino

- Un test mío contaba mal: el primer golpe ocurre al aterrizar, antes de la ventana que yo medía. Contaba 2 golpes y esperaba 3.
- Otro comprobaba "una estalactita que se rompe" pero con un marcador que no probaba el reinicio. Lo cambié por uno real: pinchos por todo el suelo hasta perder los 3 corazones, y comprobar que la trampa vuelve a estar colgando.
- Un ciclo de includes: `Room` necesitaba el dato de la estalactita y la lógica necesitaba `Room`. La solución es la de `Coin`/`CoinCollection`: el dato en un fichero pequeño y la lógica y el estado en otro.

## Tests

57 casos. Los nuevos cubren el daño y la dirección, el retroceso (distancia y altura dentro de un rango), la invulnerabilidad, la sesión entera (un golpe por ventana, tres golpes y reinicio), la máquina de la estalactita, el parseo de Tiled y el formato de texto con sus round trips.

## Siguiente

Con el daño resuelto, tocan las **frutas que curan** (generalizar `Coin` a un coleccionable con tipo) y los **enemigos**, que reutilizarán `applyDamage` y `PlayerHurt`. El backlog completo está en `docs/BACKLOG.md`.
