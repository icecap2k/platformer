# ADR-0029: Música de cueva y gotas con sonido

- Estado: Aceptada
- Fecha: 2026-11-27

## Contexto
El juego solo tenía efectos (backlog 36: música ambiente en bucle, volumen ajustable y la gota de las estalactitas). Amplía ADR-0013.

## Decisión
- `make_music.py` **sintetiza** un bucle de 32 s (zumbido, acordes, campanas con eco y viento), sin costura: lo que sobrepasa el final se suma al principio.
- `AudioPlayer` reproduce una música que vuelve a ponerse en cola (`update()` cada fotograma), con ganancia propia. `M` silencia todo.
- `-` y `+` cambian el volumen en 11 pasos (`presentation::MusicVolume`, curva cuadrática), con un aviso en pantalla; en pausa suena al 40 %. Se leen por el carácter de la tecla, no por su posición.
- **Gotas:** el renderer avisa cuando una gota visible empieza a salpicar mientras se juega; suena `drip` más bajo. No es un evento del juego: es decorado (ADR-0008).
- **Mezcla:** a petición del autor, los efectos suenan al 60 % y las gotas al 28 % (`main.cpp`).

## Alternativas descartadas
- **Música grabada o de terceros:** habría que gestionar licencias y no sería reproducible.
- **Gota como evento del dominio:** el dominio no sabe de decorado.

## Consecuencias
- El repositorio incluye un WAV de 1,4 MB.
- El volumen no se guarda entre partidas (necesita guardado, tarea 19).
- La música es la misma en el título y en todas las salas.
