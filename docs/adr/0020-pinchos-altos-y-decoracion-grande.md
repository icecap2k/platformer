# ADR-0020: Pinchos altos hundidos y decoración grande de fondo

- Estado: Aceptada
- Fecha: 2026-11-26

## Contexto
El autor pidió pinchos el doble de altos, clavados en el suelo, y una decoración más grande y bonita, también por las paredes. Al probarla pidió que fuera más suave para distinguirla de los pinchos. Sustituye el **dibujo y el tamaño de los pinchos** de ADR-0006 y los adornos de 8×8 de ADR-0007; el comportamiento de los peligros no cambia.

## Decisión
**Pinchos:**
- Dibujo de **8×16** por tile (`make_hazards.py`).
- En las salas van en una **zanja de un tile** excavada en el suelo: asoma la mitad de arriba. En las salas 05 y 09 ya estaban en un foso.
- Hitbox de **12 px** (antes 5), apoyada en el fondo de la zanja: sobresale 4 px del suelo. El símbolo `^` es ahora esa caja: ancho de un tile, tile y medio de alto, sobre el borde inferior del suyo.

**Decoración** (`presentation/decoration.h`, `make_cave_decor.py`):
- 16 piezas de entre 1×1 y 3×4 tiles que crecen del **suelo**, cuelgan del **techo** o salen de las **paredes** (dibujadas con la roca a la izquierda y espejadas al otro lado). Celdas de 32×32 en `decor.png`; la fila de abajo son las gotas.
- `sceneryFor(room)` es una **función pura de la sala** (ADR-0008): hash del identificador y de la posición. Cada pieza ocupa solo huecos, se apoya en roca, no se solapa con otra y **deja libres** pinchos, recogibles, checkpoints, salida, estalactitas trampa y el marcador.
- Para que se lea como **fondo**, el script acerca todos los colores al de la cueva y atenúa los halos; las rocas puntiagudas, las más parecidas a un pincho, se apagan más.
- Como casi no había paredes, se añadieron **paredes de roca en los lados cerrados** de las salas 03, 06, 07 y 09.

## Alternativas descartadas
- **Decoración a todo color con contorno oscuro:** se confundía con pinchos y objetos.
- **Hundir los pinchos solo en el dibujo:** el autor pidió cambiar los niveles.
- **Guardar los adornos en los mapas:** habría que colocarlos a mano en nueve salas.

## Consecuencias
- Saltar unos pinchos cuesta casi lo mismo que antes; caer en la zanja es fácil si se pisa el borde.
- Cambiar un mapa cambia su decoración, sin editar nada más.
- `PropType` y las celdas de `decor.png` deben seguir **sincronizados a mano** con el script.
