---
title: Cómo se trabaja
section: trabajar
order: 3
summary: Idiomas, ramas, decisiones y cómo mantener al día esta documentación.
---

## Idiomas

Código, comentarios, pruebas y mensajes de commit en **inglés**. Documentación, decisiones y backlog en
**español**.

## Un cambio

1. Una **rama** nueva (`main` está protegida).
2. Código y pruebas.
3. **Actualizar la documentación** de lo que haya cambiado.
4. Si cambia una decisión, un **ADR nuevo** que sustituya al anterior.
5. *Pull request*, CI en verde y fusionar.

## La documentación

Vive en `docs/src/content/docs/`, un fichero Markdown por página. Describe **cómo es el proyecto hoy**, no su
historia.

```bash
cd docs
pnpm install
pnpm dev       # http://localhost:4321
pnpm check     # comprueba código citado y enlaces
```

Los fragmentos de código llevan encima un comentario con su fichero:

```text
<!-- src: game/src/domain/physics/aabb.h -->
```

`pnpm check` verifica que esas líneas **siguen existiendo**. Si cambias el código, te avisa de qué página ha
quedado vieja. Para saltarte líneas dentro de un fragmento, escribe una línea con `// ...` (o `# ...`).

Las imágenes de las hojas de sprites se copian solas de `game/assets` al arrancar la web.

## Qué hay en cada sitio

| Quiero saber... | Sitio |
|---|---|
| cómo funciona | esta documentación |
| por qué se decidió | [Decisiones](../decisiones/) |
| qué falta | `docs/BACKLOG.md` |
| cómo se llegó hasta aquí | `docs/historial/` (archivo, no se actualiza) |
