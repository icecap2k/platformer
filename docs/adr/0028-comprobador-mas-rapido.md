# ADR-0028: Un comprobador más rápido y compilación optimizada por defecto

- Estado: Aceptada
- Fecha: 2026-11-26

## Contexto
Con las salas rediseñadas, `level_check` necesitaba 24 millones de estados, 15 s y 1,2 GB, rozando su límite; sin optimizar era unas diez veces más lento. Amplía ADR-0003 sin cambiar qué se demuestra.

## Decisión
- Los estados vistos se guardan en `StateSet`, una tabla hash de direccionamiento abierto (8 bytes por hueco).
- Lo que cada paso necesita de una sala (vecinas, bordes abiertos, objetos pendientes) se calcula una vez; los objetos ya tocados dejan de comprobarse.
- La búsqueda **para cuando todo está encontrado**; `--full` lo desactiva. Un resultado negativo sigue exigiendo explorarlo todo.
- Límite de **60 millones** de estados.
- CMake compila en **Release** si no se pide otra cosa.
- La clave de estado y la física no cambian: con `--full` explora exactamente los mismos 24 107 526 estados y da el mismo informe que antes.

## Alternativas descartadas
- **Redondear más la posición y la velocidad:** menos estados, pero se podría dar por imposible algo posible.
- **Repartir la búsqueda entre varios núcleos:** más rápido aún, pero bastante más complejo; queda para si hace falta.
- **Una librería de tablas hash:** otra dependencia para unas decenas de líneas.

## Consecuencias
- 7 s y unos 550 MB con las salas actuales.
- `states_explored` de una pasada normal ya no mide todo el espacio de estados.
- Para depurar hay que pedirlo: `-DCMAKE_BUILD_TYPE=Debug`.
