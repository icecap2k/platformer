# ADR-0001: Arquitectura en capas con el dominio sin SDL

- Estado: Aceptada
- Fecha: 2026-11-17 (decisión original en la épica E0; revisada el 2026-11-19)

## Contexto
Es el primer videojuego del autor (viene de frontend y DDD, aprende C++ y SDL3). Se quiere aprender estructurando un juego con capas claras. Se planteó la duda de si era mejor organizar por módulos/funcionalidades, como suelen hacerse los juegos.

## Decisión
Cuatro capas; las dependencias solo apuntan hacia el dominio:

```
presentation ──► application ──► domain
      │                           ▲
      └──► infrastructure ────────┘
```

- `domain`: reglas puras (jugador, física, mapas, salas, daño, trampas). **No incluye SDL**; el compilador lo hace cumplir porque la librería no enlaza con él.
- `application`: `GameSession`, un paso del juego (`update(input)`), devuelve **eventos** (`RoomChanged`, `CoinCollected`, `FruitEaten`, `PlayerHurt`, `StalactiteFell`, `GameRestarted`, etc.) en vez de actuar sobre ellos. El sonido (ADR-0013) es precisamente quien reacciona a esos eventos.
- `infrastructure`: ventana, texturas, teclado, cargador de Tiled (el cargador va en su propia librería sin SDL, `infrastructure_assets`).
- `presentation`: `GameRenderer`; la lógica visual pura (`animation`, `decoration`, `autotile`, `drip`) vive en `presentation_animation`, sin SDL, para poder probarla.
- `main.cpp` es la raíz de composición.

Dentro del dominio, las carpetas son por **tipo** (`entities`, `physics`, `rules`, `world`, `input`, `time`).

## Alternativas descartadas
- **Organizar todo por funcionalidad/módulo desde ya:** es lo habitual en juegos y mejora la localidad, pero hoy hay un jugador, monedas y 4 salas: el coste de reestructurar supera el beneficio.
- **Entidades y componentes (ECS):** solo compensa con docenas de tipos de entidad.

## Consecuencias
- Los tests nunca enlazan SDL: simulan partidas enteras sin ventana (más de 50 casos), y `level_check` reutiliza la física real (ver ADR-0003).
- Cambiar el arte (jugador, fondo, rocas, gotas) no toca la lógica.
- **Punto de revisión:** cuando lleguen los **enemigos**, una funcionalidad atravesará seis carpetas (`entities`, `physics`, `rules`, sesión, renderer, eventos). Entonces se agrupará **dentro del dominio por funcionalidad** (`domain/enemies/` con su entidad, movimiento y reglas juntos), manteniendo las capas. No se adoptará ECS salvo necesidad clara. Está en el backlog.
