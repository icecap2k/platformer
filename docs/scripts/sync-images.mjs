// Copies the game's sprite sheets into the site, so the pictures in the documentation are
// always the real ones. Runs before `dev` and `build`.
import { copyFileSync, mkdirSync } from 'node:fs';
import { fileURLToPath } from 'node:url';

const assets = new URL('../../game/assets/', import.meta.url);
const target = new URL('../public/img/sheets/', import.meta.url);
mkdirSync(target, { recursive: true });
for (const file of ['sprites/hero.png', 'sprites/rocks.png', 'sprites/sprites.png', 'sprites/enemies.png',
  'sprites/orbs.png', 'sprites/exit.png', 'sprites/decor.png', 'sprites/hazards.png', 'sprites/checkpoints.png',
  'sprites/fireflies.png', 'background/cave.png']) {
  copyFileSync(new URL(file, assets), new URL(file.split('/').pop(), target));
}
console.log(`images copied to ${fileURLToPath(target)}`);
