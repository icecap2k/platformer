---
title: "Frutas que curan, y recogibles sin duplicar código"
epic: "E10 · Frutas"
date: 2026-11-19
summary: "Una baya luminosa que devuelve un corazón. La decisión de diseño interesante no es la fruta, sino cómo añadir un recogible nuevo sin copiar el código de las monedas ni renombrar medio proyecto."
---

Tenemos corazones y daño, pero nada los recupera. Esta épica añade la **fruta que cura**: una baya que brilla y devuelve un corazón. Es pequeña, pero tenía una trampa: el backlog avisaba de **generalizar los coleccionables antes de duplicar código**.

## Qué hace

- Cura **un corazón**, sin pasar del máximo.
- Si ya tienes la salud llena, **se come igual y se desperdicia** (decisión del autor: más simple, aunque castiga el descuido). El evento lo cuenta: `hearts_gained == 0`.
- Si pierdes todos los corazones y el juego se reinicia, las frutas **vuelven a su sitio**, como las monedas y las trampas.
- Es una **baya luminosa de cueva**: rosa-naranja y con un brillo que pulsa, para que no se confunda con la moneda dorada ni con el corazón rojo del marcador.

## El problema de siempre: ¿duplico o generalizo?

Hay unos 30 ficheros que mencionan `Coin`. Las dos salidas fáciles eran malas:

1. **Copiar** todo el código de las monedas para las frutas: colección de recogidos, detección de contacto, evento, lector de Tiled... Funciona, pero cada recogible futuro lo copiaría otra vez.
2. **Renombrar `Coin` a `Collectible` con un tipo** en todos los sitios: es lo "correcto" sobre el papel, pero son 30 ficheros de cambio mecánico para añadir **un** tipo.

Elegimos un punto medio, **generalizar solo lo que de verdad se repite**:

- La **colección de recogidos** (`CoinCollection`) se llama ahora `PickupCollection`: guarda *qué* se ha recogido por sala e índice, y eso no depende de si es moneda o fruta. El nombre antiguo sigue existiendo como alias, así que nada más se tuvo que tocar. Cada tipo tiene **su propia colección**.
- La **detección de contacto** es una plantilla, `pickupsTouchedBy`: sirve para cualquier lista de cosas que tengan `bounds()`. `coinsTouchedBy` y `fruitsTouchedBy` son una línea cada una.

Lo que **no** se generalizó: `Fruit` es un tipo propio, con su lista en `Room`. Si aparece un tercer recogible, ese será el momento de introducir un `Collectible` con tipo. Esperar al tercer caso nos da información real para diseñarlo bien; con dos casos solo estaríamos adivinando. (Hay una regla de oro detrás: **la tercera vez se refactoriza**.)

## Cómo encaja

- `GameSession` come las frutas que toca justo después de las monedas y **antes** de los peligros, cura y emite `FruitEaten`. El marcador de corazones se actualiza solo, porque ya leía la salud del jugador.
- En Tiled, un objeto de clase `fruit` (la clase está en el proyecto de Tiled); en el formato de texto, `F`, tratado igual que `C`.
- **`level_check` las comprueba como a las monedas:** si una fruta no se puede alcanzar, el nivel falla. Hay frutas en `room_02` (en el suelo, justo pasados los pinchos) y en `room_04` (sobre la plataforma de la izquierda), y el comprobador confirma que se alcanzan.

## El dibujo

Cuatro fotogramas de **16×16** en filas nuevas de la hoja de sprites (los generan los mismos scripts de arte): la baya de 8×8 en el centro de un **halo cálido y semitransparente que respira**, con destellos en cruz que cambian de sitio. `fruitFrame` los cicla cada 160 ms y `fruitBob` la hace flotar un píxel arriba y abajo. El renderer centra el fotograma sobre la fruta, así que el halo se sale de su hitbox, que sigue siendo de 8×8.

### La primera versión no se veía

La primera versión tenía el brillo hecho con **unos pocos píxeles sueltos** que cambiaban cada 220 ms, y el autor ni lo notó. Una lección de arte: sobre un fondo oscuro, un resplandor necesita **tamaño y transparencia** (un halo que se mezcla con el fondo), no solo un color más claro. Lo rehicimos con un halo semitransparente que crece y se ilumina, más destellos en cruz y un pequeño balanceo, y ahora el efecto se lee de un vistazo. Sin transparencia en la hoja de sprites no habría sido posible: PNG con canal alfa y la mezcla por defecto de SDL hicieron el trabajo.

## Un fallo mío, otra vez de test

Escribí un test que comprobaba que la baya y la moneda iban desfasadas en el instante 220 ms... y justo ahí coinciden por casualidad (las dos están en el fotograma 1). El código estaba bien; la comprobación, mal elegida. Se movió a 440 ms, donde valen 2 y 3.

## Tests

Pasamos de 80 a **88 casos**: curar sin pasar del máximo, la fruta desperdiciada con salud llena, comer solo una vez, el reinicio que las repone, que monedas y frutas son independientes aunque estén en el mismo sitio, el lector de Tiled, el round trip del símbolo `F`, `level_check` con una fruta inalcanzable, la animación, y que ninguna fruta de los niveles reales esté enterrada ni flotando.

## Siguiente

Los **enemigos**, que reutilizarán `applyDamage` y `PlayerHurt` y tendrán que andar por rampas, y el **sonido**: los eventos `FruitEaten`, `PlayerHurt` y los demás ya existen y esperan quien reaccione.
