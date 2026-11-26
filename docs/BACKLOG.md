# Backlog

Tamaño: **S** ≈ medio día, **M** ≈ 1-2 días, **L** ≈ varios días. Prioridad: **P0** = ya hecho en la última épica,
**P1** = lo siguiente, **P2** = después, **P3** = pulido.

## Hecho (E8 · Peligros)

| # | Tarea |
|---|---|
| 1 | Salud (3 corazones), invulnerabilidad 1,5 s, retroceso con mini salto, pose `Hurt` y parpadeo |
| 2 | Reinicio total al llegar a 0 + eventos `PlayerHurt` / `StalactiteFell` / `GameRestarted` |
| 3 | HUD de corazones |
| 4 | Pinchos (objeto de Tiled `spikes`, sprite, `level_check` los evita, símbolo `^` en texto) |
| 5 | Estalactita trampa (`falling_stalactite`: tiembla, cae, se rompe; símbolo `V` en texto) |
| 6 | Proyecto de Tiled con las clases de objeto (`coin`, `spikes`, `falling_stalactite`) — **sin verificar en Tiled** |
| 7 | Post 13 y README |

## Hecho (E9 · Rampas)

| # | Tarea |
|---|---|
| 27 | Rampas de 45° de suelo (tiles 5 y 6, `isGround`, apoyo por el centro de los pies); la escalera de `room_01` pasa a ser una rampa de 5 tiles |
| 28 | Deslizamiento al bajar andando (2,4 px/paso, impulso 12 pasos que decae), pose `Slide` (16 fotogramas) |
| 29 | Símbolos `/` y `\` en el formato de texto; filas de rampa en `rocks.png`; paleta de Tiled con 6 tiles |
| 30 | ADR-0010 y post 14 |
| 32 | **Agacharse** (flecha abajo en suelo firme: hitbox a la mitad, 0,75 px/paso, no salta, solo se levanta con hueco encima) y **deslizar con la flecha abajo** sobre una rampa (corrige la primera versión, que deslizaba al caminar); ADR-0011 y post 15 |

## Hecho (E10 · Frutas que curan)

| # | Tarea |
|---|---|
| 34 | **Frutas que curan** (baya luminosa, 1 corazón, se come aunque la salud esté llena): `Fruit`, `PickupCollection` y detección de contacto compartidas con las monedas, evento `FruitEaten`, clase `fruit` de Tiled y símbolo `F`; `level_check` las comprueba; colocadas en `room_02` y `room_04`; ADR-0012 y post 16 |

## Hecho (E11 · Sonido)

| # | Tarea |
|---|---|
| 35 | **Sonido por eventos**: 10 efectos sintetizados (`make_sounds.py`), `soundFor` (puro) y `AudioPlayer` (SDL, mudo si no hay audio), tecla `M`, eventos nuevos (`PlayerJumped`, `PlayerLanded`, `SlideStarted`, `StalactiteWarned`, `StalactiteShattered`); ADR-0013 y post 17 |

## Hecho (E12 · Enemigos)

| # | Tarea |
|---|---|
| 9 | **Enemigos que patrullan** (`walker`, un escarabajo): caminan por el suelo y dan la vuelta ante paredes, rampas, bordes y **pinchos**; al pisarlos quedan **chafados 1 s** y desaparecen; **se pisan** (rebote, y más alto si mantienes salto) y si no, dañan por `applyDamage`; estado `EnemyPatrols`, evento `EnemyStomped`, sonido `stomp`, clase `walker` de Tiled y símbolo `E`; uno en `room_03` y otro en `room_04`; `level_check` los cuenta pero no los modela; ADR-0014 y post 18 |
| 39 | **Luciérnagas** decorativas (3 por sala, función pura del reloj, ADR-0008): quietas 3 s, vuelan en curva 5 tiles de lado y 2 abajo, con desfases propios |

## Hecho (E13 · Checkpoints)

| # | Tarea |
|---|---|
| 12 | **Checkpoints**: objeto `checkpoint` (símbolo `K`) que se toca y es donde reapareces con salud completa; se conservan monedas y frutas y se reponen enemigos y trampas de esa sala; sin checkpoint, reinicio total como antes; eventos `CheckpointReached` y `PlayerRespawned`, sonido `checkpoint`; `level_check` los comprueba; uno en `room_02` y otro en `room_03`; ADR-0015 y post 19 |

## Hecho (E14 · Título y pausa)

| # | Tarea |
|---|---|
| 11 | **Pantalla de título y pausa** (sin Game Over: ver ADR-0016): `GameFlow` por encima de `GameSession`, menús con la fuente de SDL, `P`/`Esc` pausan, "Volver al título" y "Salir", `Empezar` crea una partida limpia; `MenuCommand` como entrada de menús; ADR-0016 y post 20 |

## Hecho (E15 · Jump buffering y sacudida)

| # | Tarea |
|---|---|
| 16 | **Jump buffering** (`kJumpBufferSteps = 6`, un salto pulsado justo antes de aterrizar sale al tocar suelo) y **sacudida de pantalla** al recibir daño (función sin estado de `invulnerable_steps`); `level_check` ignora el buffer a propósito; ADR-0017 y post 21 |
| 10, 38, 15 | Comprobado por el autor: los sonidos suenan bien (incluidos `stomp` y `checkpoint`) y el proyecto de Tiled funciona |

## Hecho (E16 · Meta y más salas)

| # | Tarea |
|---|---|
| 20 | **Salida final y salas 05-09**: objeto `exit` (símbolo `X`), `GameWon`, pantalla de final con las monedas, sonido `win`, cinco salas nuevas (13 monedas, 7 escarabajos) generadas por `make_rooms_05_09.py`; `level_check` exige tocar la salida; ADR-0018 y post 22 |

## Hecho (E17 · Cambio de gráficos)

| # | Tarea |
|---|---|
| 41 | **Nuevo protagonista** (capucha azul que ondea, `make_hero_sprites.py`, misma hitbox), **escarabajos de 16×16** (dibujo y hitbox, 4 pasos), checkpoint y salida más grandes, frutas flotando un tile sobre el suelo; ADR-0019 y post 23 |

## Hecho (E18 · Pinchos y decoración)

| # | Tarea |
|---|---|
| 42 | **Pinchos de 8×16 hundidos** en zanjas (hitbox de 12 px), **16 adornos grandes** en suelo, techo y paredes, atenuados para distinguirse, y paredes de roca en los lados cerrados de las salas 03, 06, 07 y 09; ADR-0020 y post 24 |

## Hecho (E19 · Orbes)

| # | Tarea |
|---|---|
| 43 | **Tres orbes** animados que abren la salida (silueta a trazos al cogerlos, marcador, sonidos `orb` y `gate`, reja que sube); `level_check` exige tocarlos; ADR-0021 y post 25 |

## Hecho (Documentación)

| # | Tarea |
|---|---|
| 44 | **Documentación del proyecto** en lugar del blog por épica: 28 páginas por temas, código citado comprobado (`pnpm check`), ADR publicados y posts archivados en `docs/historial/`; ADR-0022 |

## Siguiente

| # | Tarea | Pri | Tam | Depende de |
|---|---|---|---|---|
| 45 | Ejecutar `pnpm check` de la documentación en el CI | P3 | S | — |
| 8 | ~~Frutas que curan~~ — hecho en E10 (ver arriba). Pendiente: un `Collectible` genérico con tipo cuando haya un tercer tipo de recogible | P3 | S | — |
| 9 | ~~Enemigos básicos~~ — hecho en E12 (ver arriba). Pendiente: gravedad y rampas para enemigos, más tipos (voladores, perseguidores), otros comportamientos | P2 | M | — |
| 36 | Música de fondo (bucle ambiente de cueva; volumen ajustable) y sonidos ambientales (la gota de las estalactitas de adorno) | P2 | M | 10 |
| 37 | Variar el sonido según el caso: fruta desperdiciada, golpe que mata, pasos sobre roca | P3 | S | 10 |
| 40 | Menús: sonidos al moverse/elegir, opciones (volumen), mando y fuente propia con acentos | P3 | M | 18 |
| 13 | **Rendimiento de `level_check`**: con la compilación por defecto tarda ~45 s, pero en **Release tarda ~4 s** (probado); decidir si `cmake` usa Release por defecto, y la poda sigue haciendo falta cuando crezca | P2 | S | — |
| 14 | ~~CI en GitHub Actions~~ — hecho (`.github/workflows/ci.yml`: macOS, SDL3 con Homebrew, build Release y `ctest`; pasó en el PR #7). Pendiente: activar la protección de `main` exigiéndolo | P2 | S | — |
| 17 | Partículas: polvo al aterrizar, destello de moneda, **fragmentos al romperse la estalactita** | P2 | S | — |
| 18 | **Mando** (`GamepadInputMapper`; decidir cómo combinar con el teclado) | P2 | M | — |
| 19 | **Guardado de partida** (usar el `id` del objeto de Tiled, no el índice) | P2 | M | 8, 11 |
| 21 | Pinchos en techo y en paredes (orientación), plataformas de un solo sentido, puertas con llave, tiles nuevos | P2 | M | — |
| 22 | Que `level_check` también tenga en cuenta las estalactitas (hoy se ignoran) o que se marque en el informe | P2 | M | 13 |
| 23 | Reorganizar `domain/` por funcionalidad cuando lleguen los enemigos (`domain/enemies/`, …) | P2 | S | 9 |
| 24 | Pulido de ventana: pantalla completa, escala configurable, pausa al perder el foco | P3 | S | — |
| 25 | Remapeo de teclas y accesibilidad (daltonismo, reducir parpadeo) | P3 | S | 18 |
| 33 | **Enseñar a `level_check` a agacharse y deslizarse** (hoy no pulsa abajo: un nivel que exija agacharse se daría por inalcanzable) | P2 | M | — |
| 31 | Rampas: pinchos y enemigos sobre una rampa, rampas de techo y de otros ángulos, y el hundimiento de 4 px del pie delantero | P3 | M | 9 |

## Riesgos e ideas a vigilar

- Los checkpoints no se guardan en disco (tarea 19) y reaparecer sin invulnerabilidad puede ser duro si el checkpoint está junto a un peligro.
- Cada estado nuevo (salud, invulnerabilidad, estalactitas) agranda la búsqueda de `level_check`; ya pasó con el salto (tarea 13).
- Generalizar los coleccionables (tarea 8) **antes** de duplicar código para las frutas.
- Los enemigos tampoco están en `level_check`: un nivel podría exigir pisarlos o esquivarlos sin que el comprobador lo sepa.
- `level_check` explora ~16 M de estados (límite 20 M): otra sala o mecánica puede pasarse; ver tarea 13.
- `level_check` comprueba cada orbe por separado, no el orden: al añadir salas, lanzarlo también desde la salida (ADR-0021).
- Las estalactitas no están en el análisis de alcanzabilidad: un nivel podría ser "ganable" según `level_check` y exigir esquivarlas.
