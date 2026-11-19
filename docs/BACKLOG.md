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

## Siguiente

| # | Tarea | Pri | Tam | Depende de |
|---|---|---|---|---|
| 8 | **Frutas que curan**: generalizar `Coin` a un coleccionable con tipo (moneda, fruta que cura 1) | P1 | M | — |
| 9 | **Enemigos** básicos (patrulla; reutilizan `applyDamage` y `PlayerHurt`; decidir si se pueden pisar) | P1 | L | — |
| 10 | **Sonido**: `AudioPlayer` en infraestructura que reacciona a los eventos (moneda, daño, salto, gota, estalactita) | P1 | M | — (los eventos ya existen) |
| 11 | **Pantalla de título, pausa y Game Over** (estado por encima de `GameSession`) | P1 | M | — |
| 12 | **Checkpoints / reaparecer en la sala**: suavizar el reinicio total si resulta frustrante | P1 | M | — |
| 13 | **Rendimiento de `level_check`**: ya tarda ~40-65 s y crece con cada sala y cada estado nuevo; poda/ordenación o etiqueta de test lento | P1 | S | — |
| 14 | **CI** en GitHub Actions (build + `ctest`) e instalar `gh` para abrir PRs desde la terminal | P1 | S | 13 |
| 15 | **Verificar el proyecto de Tiled** abriendo `game/assets/tiled/platformer.tiled-project` y colocando un pincho a mano | P1 | S | — |
| 16 | **Jump buffering** (la revisión externa lo sugirió junto al coyote time) y *screen shake* al recibir daño | P2 | S | — |
| 17 | Partículas: polvo al aterrizar, destello de moneda, **fragmentos al romperse la estalactita** | P2 | S | — |
| 18 | **Mando** (`GamepadInputMapper`; decidir cómo combinar con el teclado) | P2 | M | — |
| 19 | **Guardado de partida** (usar el `id` del objeto de Tiled, no el índice) | P2 | M | 8, 11 |
| 20 | Más salas y progresión (room_05+, meta, final) | P2 | L | 9 |
| 21 | Pinchos en techo y en paredes (orientación), plataformas de un solo sentido, puertas con llave, tiles nuevos | P2 | M | — |
| 22 | Que `level_check` también tenga en cuenta las estalactitas (hoy se ignoran) o que se marque en el informe | P2 | M | 13 |
| 23 | Reorganizar `domain/` por funcionalidad cuando lleguen los enemigos (`domain/enemies/`, …) | P2 | S | 9 |
| 24 | Pulido de ventana: pantalla completa, escala configurable, pausa al perder el foco | P3 | S | — |
| 25 | Remapeo de teclas y accesibilidad (daltonismo, reducir parpadeo) | P3 | S | 18 |
| 26 | Actualizar los posts 10 y 12 del blog, que quedaron con detalles anteriores a los cambios de arte y de pinchos | P3 | S | — |

## Riesgos e ideas a vigilar

- El reinicio total es duro: si se siente frustrante, subir la tarea 12 (checkpoints).
- Cada estado nuevo (salud, invulnerabilidad, estalactitas) agranda la búsqueda de `level_check`; ya pasó con el salto (tarea 13).
- Generalizar los coleccionables (tarea 8) **antes** de duplicar código para las frutas.
- Las estalactitas no están en el análisis de alcanzabilidad: un nivel podría ser "ganable" según `level_check` y exigir esquivarlas.
