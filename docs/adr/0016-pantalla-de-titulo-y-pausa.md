# ADR-0016: Pantalla de título y pausa por encima de la partida

- Estado: Aceptada
- Fecha: 2026-11-23

## Contexto
El juego arrancaba directamente en la partida y `Esc` salía sin más. Con checkpoints (ADR-0015) un Game Over deja de tener mucho sentido, pero hacían falta un título y una pausa.

## Decisión
- **`GameFlow`** (`application/game_flow.h`, sin SDL) es el estado de los menús **por encima** de `GameSession`: `Screen {Title, Playing, Paused}`, el menú de cada pantalla (`MenuItem`: Start, Continue, BackToTitle, Quit), la selección (que da la vuelta) y `quitRequested()`. `handle(MenuCommand)` devuelve `true` solo cuando hay que **empezar una partida nueva** (Start).
- **`GameSession` no sabe que hay menús.** `main.cpp` guarda un `std::optional<GameSession>`: se crea al elegir Start (con un mundo recién cargado) y se ignora con Volver al título; así "Empezar" siempre es una partida limpia. Mientras no se está en `Playing` el reloj corre pero **no se da ningún paso** (la pausa congela también estalactitas, enemigos y trampas; las luciérnagas y el brillo siguen porque dependen del reloj, como el resto de adornos).
- **Entrada:** `domain::MenuCommand {Up, Down, Confirm, Back, Pause}` son pulsaciones sueltas (no teclas mantenidas, a diferencia de `InputState`) que `KeyboardInputMapper::takeMenuCommands()` entrega por orden. Teclas: arriba/abajo o W/S, Enter o Espacio, `Esc` (atrás) y `P` (pausa). **`Esc` ya no sale directamente:** pausa, y en el título sale; cerrar la ventana sigue saliendo siempre.
- **Pausa:** `P` o `Esc` pausan y reanudan; el menú ofrece Continuar, Volver al título y Salir.
- **Dibujo:** texto con la **fuente integrada de SDL** (`SDL_RenderDebugText`, 8×8), ampliada con `SDL_SetRenderScale` para los títulos, sin fuente propia: el título es el fondo de la cueva, "PLATFORMER" y el menú; la pausa atenúa el juego congelado con un velo translúcido y pone su menú encima. El elegido va entre `> <` y en amarillo.
- `GameRenderer::draw(session*, flow)` dibuja la pantalla actual; `session` puede ser nulo solo en el título.

## Alternativas descartadas
- **Meter el estado de menú dentro de `GameSession`:** mezclaría reglas de juego con interfaz y complicaría los tests.
- **Pantalla de Game Over:** no hace falta con checkpoints y reinicio total; si algún día hay vidas limitadas, se añade como otra `Screen`.
- **Fuente propia en `.png`:** más trabajo y un script más; la integrada basta por ahora (solo mayúsculas y sin acentos, 8 px).

## Consecuencias
- Queda sin hacer: **sonidos de menú**, estadísticas en el título (requieren guardado, tarea 19), opciones y mando (`MenuCommand` ya es el sitio donde mapearlo).
- El texto es solo inglés/mayúsculas; si se quiere español con acentos, habrá que dibujar una fuente.
