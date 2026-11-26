// Keeps the documentation honest:
//   1. every code block preceded by `<!-- src: path -->` must still be in that file, line by line
//      and in order (indentation is ignored; a line with only `// ...` skips any number of lines);
//   2. every internal link (`../page/`, `../decisiones/id/`) and image (`/img/...`) must exist.
// Run it with `pnpm check` (it exits with 1 and lists what is wrong).
import { existsSync, readdirSync, readFileSync } from 'node:fs';

const root = new URL('../../', import.meta.url);
const docs = new URL('docs/src/content/docs/', root);
const pages = readdirSync(docs).filter((f) => f.endsWith('.md'));
const ids = new Set(pages.map((f) => f.replace(/\.md$/, '')));
const decisions = new Set(readdirSync(new URL('docs/adr/', root)).map((f) => f.replace(/\.md$/, '')));
const problems = [];
let snippets = 0;

const clean = (text) => text.split('\n').map((l) => l.trim()).filter(Boolean);

for (const page of pages) {
  const text = readFileSync(new URL(page, docs), 'utf8');

  for (const [, path, code] of text.matchAll(/<!-- src: (\S+) -->\n```[^\n]*\n([\s\S]*?)\n```/g)) {
    snippets += 1;
    const file = new URL(path, root);
    if (!existsSync(file)) {
      problems.push(`${page}: no existe ${path}`);
      continue;
    }
    const source = clean(readFileSync(file, 'utf8'));
    let from = 0;
    // Each run of lines between `// ...` markers must appear contiguously, after the previous one.
    for (const run of code.split(/^\s*(?:\/\/|#) \.\.\.\s*$/m).map(clean).filter((r) => r.length)) {
      let at = -1;
      for (let i = from; i + run.length <= source.length && at < 0; i += 1) {
        if (run.every((line, k) => source[i + k] === line)) at = i;
      }
      if (at < 0) {
        problems.push(`${page}: el código ya no coincide con ${path}: "${run[0]}"`);
        break;
      }
      from = at + run.length;
    }
  }

  for (const [, target] of text.matchAll(/\]\((\.\.\/[^)#\s]+)(?:#[^)]*)?\)/g)) {
    const [, first, second] = target.split('/');
    const ok = first === 'decisiones' ? !second || decisions.has(second) : ids.has(first);
    if (!ok) problems.push(`${page}: enlace roto ${target}`);
  }

  // Images are served from docs/public (the sprite sheets are copied there from the game).
  for (const [, target] of text.matchAll(/!\[[^\]]*\]\(([^)\s]+)\)/g)) {
    const file = target.replace('/img/sheets/', 'public/img/sheets/');
    if (!target.startsWith('/img/') || !existsSync(new URL(`docs/${file.replace(/^\//, 'public/')}`, root))) {
      problems.push(`${page}: imagen rota ${target}`);
    }
  }
}

if (problems.length) {
  console.error(problems.join('\n'));
  process.exit(1);
}
console.log(`docs OK: ${pages.length} páginas, ${snippets} fragmentos de código comprobados`);
