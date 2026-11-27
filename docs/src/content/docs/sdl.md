---
title: Ventana, texturas, teclado y mando
section: infraestructura
order: 1
summary: Los adaptadores que hablan con SDL3, y cómo teclado y mando dicen lo mismo.
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

## Mando: el mismo idioma

Aquí se ve para qué sirven las [capas](../capas/). El mando es **otro adaptador** que produce el mismo
`InputState` y los mismos `MenuCommand`: ni el dominio ni la partida han cambiado para soportarlo.

Está partido en dos para poder probarlo sin un mando:

| Pieza | Qué hace | ¿SDL? |
|---|---|---|
| `GamepadInputMapper` | abre los mandos que se conectan y copia sus botones en una foto (`GamepadSnapshot`) | Sí |
| `gamepad_mapping` | decide qué **significa** esa foto | No (con pruebas) |

### La zona muerta

Un stick nunca descansa exactamente en el centro. Solo cuenta si se empuja lo suficiente, y para agacharse más que
para correr, así correr con el stick un poco bajo no te agacha:

<!-- src: game/src/infrastructure/input/gamepad_mapping.h -->
```cpp
inline constexpr float kStickRunThreshold = 0.35F;
// Further to crouch (and to move in a menu), so running with the stick slightly low does not.
inline constexpr float kStickDownThreshold = 0.65F;
```

### Dos dispositivos a la vez

<!-- src: game/src/infrastructure/input/gamepad_mapping.cpp -->
```cpp
    input.left = first.left || second.left;
    input.right = first.right || second.right;
    input.jump = first.jump || second.jump;
    input.down = first.down || second.down;
```

Una acción está activa si **cualquiera** de los dos la pide. No hay que elegir dispositivo.

### Pulsar frente a mantener

En los menús importa el instante en que se pulsa. Como el mando se lee una vez por fotograma, basta comparar la
foto anterior con la actual:

<!-- src: game/src/infrastructure/input/gamepad_mapping.cpp -->
```cpp
    const auto pressed = [](bool was, bool is) { return is && !was; };
```

Si se desenchufa el último mando jugando, `main` pone la pausa.
