# ADR-0026: Transiciones con easing entre salas y pantallas

- Estado: Aceptada
- Fecha: 2026-11-26

## Contexto
El cambio de sala y de pantalla era instantáneo. El autor pidió que fuera con *easing*, en los dos casos.

## Decisión
- **Entre salas:** deslizamiento de 450 ms hacia el lado por el que se sale; la imagen anterior se va y la nueva la sigue sin hueco.
- **Entre pantallas** (título, juego, final) y al **reaparecer** o reiniciar: fundido de 600 ms. Pausar no tiene transición.
- Curva `easeInOut` cúbica (`presentation/transition.h`, lógica pura y probada).
- La escena se dibuja en texturas fuera de pantalla (`infrastructure::RenderTarget`); se conserva la última para salir de ella. El marcador y la pausa se pintan encima y no se mueven.
- El renderer se entera por los eventos (`onEvent`); **la simulación se detiene** mientras dura (`inTransition()`).

## Alternativas descartadas
- **Dibujar las dos salas en vivo:** obliga a que todo el dibujado acepte cualquier sala; con una foto de la anterior basta.
- **Seguir simulando durante la transición:** el jugador actuaría sin ver dónde está.

## Consecuencias
- Cada cambio de sala detiene el juego casi medio segundo.
- La sala que se abandona queda congelada mientras se va.
