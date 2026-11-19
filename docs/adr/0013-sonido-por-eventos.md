# ADR-0013: Sonido por eventos y efectos sintetizados

- Estado: Aceptada
- Fecha: 2026-11-19

## Contexto
Todo lo construido (daño, frutas, monedas, rampas, trampas) se jugaba en silencio. Los eventos del juego (`GameEvent`) ya existían y esperaban a alguien que reaccionara a ellos (ADR-0001). Había que decidir de dónde salen los sonidos, si había música y dónde vive la lógica de "qué suena cuándo" sin ensuciar el dominio.

## Decisión
**Arquitectura:** el sonido es una capa más de **presentación** que reacciona a los eventos; las reglas del juego no saben que existe el audio.
- `presentation/sound_cues.{h,cpp}` (lógica pura, sin SDL, en `presentation_animation`): `soundFor(evento) → optional<Sound>`, `soundName` y `allSounds`. Es un `std::visit`, así que **añadir un evento sin decidir su sonido no compila**. Cambiar room es silencioso.
- `infrastructure/audio/audio_player.{h,cpp}` (SDL): no conoce el juego; recibe un **nombre** y reproduce `assets/sounds/<nombre>.wav`. Cada sonido tiene **su propio stream de audio enlazado a un único dispositivo**, así que suenan mezclados y repetir uno lo reinicia. Si no hay dispositivo o falta un fichero, avisa una vez y **se queda en silencio: el juego nunca falla por el sonido**.
- `main.cpp` los une: por cada evento llama a `soundFor` y a `audio.play`. La tecla **`M`** silencia/activa (`KeyboardInputMapper::takeMuteToggle`; es de la aplicación, no una acción del juego, como salir).

**Eventos nuevos** que faltaban para poder sonar: `PlayerJumped`, `PlayerLanded{impact_speed}`, `SlideStarted`, `StalactiteWarned` y `StalactiteShattered` (junto a los `StalactiteFell`, `CoinCollected`, `FruitEaten`, `PlayerHurt` y `GameRestarted` que ya había).
- `stepPlayer` devuelve ahora un `StepResult{jumped}` (los llamadores que lo ignoran no cambian).
- El **aterrizaje solo se anuncia** si la caída lleva al menos 1,5 px/paso (`kLandingSoundSpeed`), para que andar por escalones y rampas no suene como una cadena de golpes.

**Sonidos (10):** salto, aterrizaje, moneda, fruta, daño, deslizamiento, aviso de estalactita (retumbo), estalactita que se suelta (chasquido), estalactita que se rompe (estruendo) y reinicio (jingle descendente).

**Origen de los efectos: generados por script** (`game/tools/audio/make_sounds.py`, solo biblioteca estándar: osciladores, barridos de tono, ruido filtrado y envolventes), como el arte (ADR-0007). WAV mono de 16 bits a 22,05 kHz, deterministas (el ruido usa semillas fijas) y normalizados al 80 % de la escala para que no saturen. Sin licencias ni ficheros de terceros.

**Pruebas:** `sound_cues_test` (cada evento tiene su sonido, nombres distintos, todo sonido de evento está en la lista de carga) y `sound_assets_test`, que lee cada WAV con un lector RIFF mínimo y comprueba formato, duración (0,05-1,5 s), que no esté vacío ni sature y que termine sin chasquido. Los eventos nuevos se prueban en la sesión.

## Alternativas descartadas
- **Reutilizar los WAV de Coffee Crisis** (`jump.wav`, `item_pickup.wav`…): rápido y con más carácter, pero hay que revisar su licencia y no existen sonidos para pinchos, estalactitas ni rampas.
- **Música de fondo:** se deja para una tarea aparte; añadiría bucles, volumen y mezcla, y decodificar OGG si se reutilizara `playing.ogg`.
- **Decidir el sonido dentro del dominio o de `GameSession`:** acoplaría las reglas al audio.

## Consecuencias
- **No he podido oír los sonidos** al construirlos (se comprueban por formato y energía, no por oído): su afinación es lo primero que habrá que ajustar de oído. Se escuchan con `afplay game/assets/sounds/<nombre>.wav`, y se retocan en `make_sounds.py`.
- Hay un sonido por evento y no se distingue, por ejemplo, la fruta desperdiciada de la útil ni el golpe que mata del que no; `FruitEaten.hearts_gained` y `PlayerHurt.health_left` lo permiten más adelante.
- Fuera de alcance por ahora: música, volumen ajustable y sonidos ambientales (la gota de las estalactitas de adorno es solo decoración, sin evento).
