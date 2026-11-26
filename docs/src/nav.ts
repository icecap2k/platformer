import { getCollection } from 'astro:content';

// Sections in reading order.
export const SECTIONS = [
  { id: 'empezar', title: 'Empezar' },
  { id: 'arquitectura', title: 'Arquitectura' },
  { id: 'dominio', title: 'Dominio' },
  { id: 'aplicacion', title: 'Aplicación' },
  { id: 'infraestructura', title: 'Infraestructura' },
  { id: 'presentacion', title: 'Presentación' },
  { id: 'niveles', title: 'Niveles' },
  { id: 'trabajar', title: 'Trabajar en el proyecto' },
] as const;

export const base = import.meta.env.BASE_URL.replace(/\/?$/, '/');

export async function navigation() {
  const pages = await getCollection('docs');
  const sections = SECTIONS.map((section) => ({
    ...section,
    pages: pages.filter((p) => p.data.section === section.id).sort((a, b) => a.data.order - b.data.order),
  }));
  return { sections, flat: sections.flatMap((s) => s.pages) };
}
