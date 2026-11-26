---
title: Ventana, texturas y teclado
section: infraestructura
order: 1
summary: Los adaptadores que hablan con SDL3.
---

La infraestructura es el único sitio (junto al renderer) que conoce **SDL3**, la librería que abre ventanas,
lee el teclado y reproduce sonido en cada sistema operativo.

## `Window`: un lienzo pequeño ampliado

El juego dibuja siempre en **320×240**. SDL lo amplía por un **número entero** (×3), para que los píxeles
sigan cuadrados y nítidos:

<!-- src: game/src/infrastructure/sdl/window.cpp -->
```cpp
    SDL_SetRenderLogicalPresentation(renderer_, logical_width, logical_height,
                                     SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
```

Todo el código de dibujo piensa en 320×240 y se olvida del tamaño real.

## RAII: crear y destruir sin olvidos

SDL pide destruir a mano lo que creas. Se envuelve en una clase cuyo **destructor** lo hace:

<!-- src: game/src/infrastructure/sdl/window.cpp -->
```cpp
Window::~Window() {
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);
    SDL_Quit();
}
```

Cuando `main` termina, incluso por un error, la limpieza ocurre sola. La línea
`Window(const Window&) = delete;` prohíbe copiar la ventana: dos copias intentarían destruir lo mismo.

## `Texture`: una imagen en la tarjeta gráfica

Carga un PNG con `stb_image` y lo convierte en textura, con escalado "al vecino más cercano" (sin
difuminar):

<!-- src: game/src/infrastructure/sdl/texture.cpp -->
```cpp
    SDL_SetTextureScaleMode(texture_, SDL_SCALEMODE_NEAREST);
```

Su método `draw` copia **un rectángulo de la imagen** a **un rectángulo de la pantalla**, opcionalmente en
espejo. Con eso se dibuja el juego entero ([Cómo se dibuja](../renderizado/)).

## Teclado: de teclas a intenciones

`KeyboardInputMapper` ofrece dos vistas del teclado:

| Método | Para qué | Comportamiento |
|---|---|---|
| `state()` | jugar | qué teclas están **mantenidas** |
| `takeMenuCommands()` | menús | cada **pulsación**, una vez |

<!-- src: game/src/infrastructure/input/keyboard_input_mapper.cpp -->
```cpp
        } else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
```

`!event.key.repeat` descarta la autorrepetición del sistema.
