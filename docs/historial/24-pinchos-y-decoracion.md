---
title: "Pinchos que se clavan y una cueva con más vida"
epic: "E18 · Pinchos y decoración"
date: 2026-11-26
summary: "Pinchos el doble de altos hundidos en el suelo, dieciséis adornos grandes en suelo, techo y paredes, y por qué hubo que apagarlos."
---

Dos cambios de aspecto que acabaron tocando los niveles.

## Pinchos hundidos

Ahora miden **8×16** y están metidos en una **zanja de un tile**, así que asoma la mitad de arriba. La hitbox creció a 12 px, pero como empieza en el fondo de la zanja solo sobresale 4 px del suelo: saltarlos cuesta casi lo mismo que antes. `level_check`, que trata los pinchos como letales, confirmó que todo sigue siendo alcanzable.

## Decoración que se coloca sola

Los adornos de 8×8 dejaron paso a **16 piezas** de hasta 3×4 tiles: setas gigantes, cristales, estalagmitas, lianas, raíces, setas en repisa... Siguen sin guardarse en los mapas. `sceneryFor(sala)` recorre los tiles y, según un hash, intenta plantar una pieza si:

- sus tiles están vacíos y no pisa otra pieza,
- tiene roca donde apoyarse (debajo, encima o a un lado),
- y no tapa nada importante: pinchos, monedas, orbes, checkpoints, la salida o el marcador.

Esas reglas se prueban sin abrir una ventana. Como casi no había paredes donde colgar nada, las salas 03, 06, 07 y 09 ganaron paredes de roca en sus lados cerrados.

## La lección: el fondo tiene que parecer fondo

La primera versión era preciosa y confusa: una estalagmita clara con contorno oscuro **parecía un pincho**. La solución fue una línea de idea en el script: acercar cada color al del fondo de la cueva, y apagar aún más las rocas puntiagudas. Los pinchos, los enemigos y las monedas conservan el color puro y el contorno, y vuelven a saltar a la vista.
