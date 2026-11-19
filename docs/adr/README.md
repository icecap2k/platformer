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
| [0006](0006-peligros-pinchos-y-estalactita-trampa.md) | Peligros: pinchos y estalactita trampa | Aceptada |
| [0007](0007-arte-generado-por-scripts.md) | Arte generado por scripts, y estilo visual | Aceptada |
| [0008](0008-efectos-visuales-puros.md) | Efectos visuales como funciones puras; paralaje descartado | Aceptada |
| [0009](0009-convenciones-de-trabajo-y-pruebas.md) | Convenciones de trabajo, pruebas y git | Aceptada |
| [0010](0010-rampas-de-45-y-deslizamiento.md) | Rampas de 45° y deslizamiento | Aceptada (el disparador, sustituido por 0011) |
| [0011](0011-agacharse-y-deslizar-con-la-flecha-abajo.md) | Agacharse y deslizar con la flecha abajo | Aceptada |
| [0012](0012-frutas-que-curan-y-recogibles.md) | Frutas que curan y recogibles | Aceptada |

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
