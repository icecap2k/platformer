# ADR-0023: Fondo más oscuro y setas luminosas animadas

- Estado: Aceptada
- Fecha: 2026-11-26

## Contexto
El autor pidió un fondo más oscuro, con degradado y setas luminosas animadas. Amplía ADR-0008: el fondo sigue siendo una imagen fija y sin paralaje; lo nuevo se mueve en el sitio.

## Decisión
- `cave.png` pasa a un **degradado** de casi negro a azul verdoso, con tramado ordenado para evitar franjas, y rocas lejanas más oscuras (`make_cave_background.py`).
- **Setas** (`make_glow_mushrooms.py`, `glow_mushrooms.png`): 3 tipos, celdas de 48×64 y 8 fotogramas (luz que pulsa, sombrero que respira, esporas). Sin contorno y apagadas.
- `backdropFor(sala)` (`presentation/backdrop.h`) las coloca por hash: como mucho una por quinto de pantalla, sobre el suelo del tercio inferior; se dibujan entre el fondo y la roca.

## Alternativas descartadas
- **Pintarlas en `cave.png`:** no podrían animarse ni adaptarse al suelo de cada sala.
- **Paralaje:** descartado en ADR-0008.

## Consecuencias
- Hay más luz en la parte baja de la pantalla, donde están pinchos y enemigos: hay que vigilar el contraste.
- La pantalla de título usa el fondo nuevo, sin setas.
