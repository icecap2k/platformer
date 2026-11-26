import { defineCollection } from 'astro:content';
import { glob } from 'astro/loaders';
import { z } from 'astro/zod';

// The documentation of the project: one Markdown file per page.
const docs = defineCollection({
  loader: glob({ pattern: '*.md', base: './src/content/docs' }),
  schema: z.object({
    title: z.string(),
    section: z.enum([
      'empezar', 'arquitectura', 'dominio', 'aplicacion', 'infraestructura', 'presentacion', 'niveles', 'trabajar',
    ]),
    order: z.number(), // position inside its section
    summary: z.string(),
  }),
});

// The architecture decision records are published as they are, straight from docs/adr.
const adr = defineCollection({
  loader: glob({ pattern: '0*.md', base: './adr' }),
});

export const collections = { docs, adr };
