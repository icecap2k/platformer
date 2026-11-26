# ADR-0022: Documentación del proyecto en lugar de un blog por épica

- Estado: Aceptada
- Fecha: 2026-11-26

## Contexto
Cada épica añadía un post. Tras 19, para saber **cómo funciona hoy** una parte había que leer varios y descartar lo que ya había cambiado. El autor pidió documentación del proyecto, fácil de leer y que sirva para aprender. Sustituye la parte de ADR-0009 que pedía un post de blog por épica.

## Decisión
- El sitio de `docs/` (Astro) es **documentación por temas**: empezar, arquitectura, dominio, aplicación, infraestructura, presentación, niveles y cómo trabajar. Una página Markdown por tema en `docs/src/content/docs/`, en español y pensada para quien aprende.
- Describe **el estado actual**. Cada cambio **actualiza las páginas afectadas** en la misma rama.
- El código citado lleva `<!-- src: fichero -->`; `pnpm check` (y `pnpm build`) fallan si esas líneas ya no existen o si un enlace está roto.
- Las hojas de sprites se copian de `game/assets` al arrancar o construir el sitio.
- Los ADR se publican tal cual en la sección «Decisiones».
- Los 26 posts quedan en `docs/historial/`, sin publicar ni actualizar.

## Alternativas descartadas
- **Mantener el blog y añadir documentación aparte:** dos cosas que escribir y una envejece.
- **Borrar los posts:** son la historia del aprendizaje; archivarlos no cuesta nada.
- **Un generador de documentación con más funciones (Starlight, por ejemplo):** otra dependencia; el sitio actual basta.

## Consecuencias
- La comprobación detecta código que ya no existe, **no explicaciones desfasadas**: hay que revisar el texto al cambiar algo.
- El CI todavía no ejecuta `pnpm check`.
