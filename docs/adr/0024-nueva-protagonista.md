# ADR-0024: La protagonista de pelo blanco

- Estado: Aceptada
- Fecha: 2026-11-26

## Contexto
El autor pidió cambiar el diseño: una protagonista con **pelo blanco largo** que se mueva como lo hacía la capucha, sin capucha, **pantalones rojos** y **chaqueta negra** con **camiseta gris**. Primero la pidió sin ojos ni boca y en estilo *low poly*; después aportó una **imagen de referencia** para seguirla. Sustituye el diseño del protagonista de ADR-0019; el resto de ese ADR sigue igual.

## Decisión
- Figura pequeña y compacta de unos 12×18 píxeles, copiada de la referencia: cabeza grande, **ojos oscuros** sin boca, horquilla roja, chaqueta abierta, pantalones rojos, botas marrones y contorno negro grueso (`make_hero_sprites.py`).
- La cabeza es arte ASCII; brazos, piernas y un **mechón que ondea** (4 fases) se colocan por pose.
- Las celdas siguen siendo de 32×28 y la hitbox de 8×16; la carrera pasa de 6 a **8 fotogramas** (19 poses × 4 fases), con las piernas dibujadas a mano, brazos que se mueven poco y una melena más tranquila. `hoodFlutter` pasa a llamarse `hairFlutter`.

## Alternativas descartadas
- **Low poly con polígonos facetados y cara sin ojos:** se probó; el autor prefirió la referencia.

## Consecuencias
- La chaqueta negra con contorno negro destaca poco sobre el fondo oscuro: la silueta la dan el pelo, la cara y los pantalones.
