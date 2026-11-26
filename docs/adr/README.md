# Registro de decisiones de arquitectura (ADR)

Cada fichero recoge **una decisión** que conviene recordar en futuras sesiones: qué se decidió, por qué, qué
alternativas se descartaron y qué consecuencias tiene. Si una decisión cambia, no se reescribe: se crea un ADR
nuevo que la sustituye y el antiguo pasa a `Sustituido por ADR-NNNN`.

Idioma: español (como el resto de la documentación). El código, los comentarios y los mensajes de commit van en inglés.

## Índice

| ADR | Decisión | Estado |
|---|---|---|
| [0001](0001-arquitectura-en-capas.md) | Arquitectura en capas con el dominio sin SDL | Aceptada |
| [0002](0002-niveles-en-tiled-y-formato-de-texto.md) | Niveles en Tiled (`.tmj`) y formato de texto como ayuda | Aceptada |
| [0003](0003-comprobador-de-alcanzabilidad.md) | Comprobador de niveles que reutiliza la física real | Aceptada |
| [0004](0004-sensacion-del-salto.md) | Sensación del salto: altura variable, coyote time, pulsación nueva | Aceptada |
| [0005](0005-salud-dano-y-reinicio.md) | Salud, daño con retroceso y reinicio total | Aceptada |
| [0006](0006-peligros-pinchos-y-estalactita-trampa.md) | Peligros: pinchos y estalactita trampa | Aceptada (dibujo y tamaño de los pinchos, sustituidos por 0020) |
| [0007](0007-arte-generado-por-scripts.md) | Arte generado por scripts, y estilo visual | Aceptada (el jugador, sustituido por 0019; los adornos, por 0020) |
| [0008](0008-efectos-visuales-puros.md) | Efectos visuales como funciones puras; paralaje descartado | Aceptada |
| [0009](0009-convenciones-de-trabajo-y-pruebas.md) | Convenciones de trabajo, pruebas y git | Aceptada (el post por épica, sustituido por 0022) |
| [0010](0010-rampas-de-45-y-deslizamiento.md) | Rampas de 45° y deslizamiento | Aceptada (el disparador, sustituido por 0011) |
| [0011](0011-agacharse-y-deslizar-con-la-flecha-abajo.md) | Agacharse y deslizar con la flecha abajo | Aceptada |
| [0012](0012-frutas-que-curan-y-recogibles.md) | Frutas que curan y recogibles | Aceptada |
| [0013](0013-sonido-por-eventos.md) | Sonido por eventos y efectos sintetizados | Aceptada |
| [0014](0014-enemigos-que-patrullan-y-se-pisan.md) | Enemigos que patrullan y se pisan | Aceptada (tamaño y dibujo, sustituidos por 0019) |
| [0015](0015-checkpoints.md) | Checkpoints y reaparición | Aceptada |
| [0016](0016-pantalla-de-titulo-y-pausa.md) | Pantalla de título y pausa por encima de la partida | Aceptada |
| [0017](0017-jump-buffering-y-sacudida-de-pantalla.md) | Jump buffering y sacudida de pantalla | Aceptada |
| [0018](0018-salida-final-y-salas-5-a-9.md) | Salida final y salas 05 a 09 | Aceptada (tamaño de la salida, sustituido por 0019; la condición de victoria, por 0021; el script de salas, por 0027) |
| [0019](0019-graficos-mas-grandes-y-nuevo-protagonista.md) | Nuevo protagonista y gráficos más grandes que su hitbox | Aceptada (el diseño del protagonista, sustituido por 0024) |
| [0020](0020-pinchos-altos-y-decoracion-grande.md) | Pinchos altos hundidos y decoración grande de fondo | Aceptada |
| [0021](0021-orbes-que-abren-la-salida.md) | Tres orbes que abren la salida | Aceptada |
| [0022](0022-documentacion-del-proyecto.md) | Documentación del proyecto en lugar de un blog por épica | Aceptada |
| [0023](0023-fondo-oscuro-y-setas-luminosas.md) | Fondo más oscuro y setas luminosas animadas | Aceptada |
| [0024](0024-nueva-protagonista.md) | La protagonista de pelo blanco | Aceptada |
| [0025](0025-pantalla-de-titulo-con-imagen.md) | Pantalla de título con un dibujo del autor | Aceptada |
| [0026](0026-transiciones-con-easing.md) | Transiciones con easing entre salas y pantallas | Aceptada |
| [0027](0027-las-salas-se-editan-en-tiled.md) | Las salas se editan en Tiled | Aceptada |

## Plantilla

```markdown
# ADR-NNNN: Título corto

- Estado: Propuesta | Aceptada | Sustituida por ADR-MMMM
- Fecha: AAAA-MM-DD

## Contexto
## Decisión
## Alternativas descartadas
## Consecuencias
```

El trabajo pendiente está en [`../BACKLOG.md`](../BACKLOG.md).
