# ADR-0031: Murciélagos, y escarabajos con gravedad y rampas

- Estado: Aceptada
- Fecha: 2026-11-27

## Contexto
Backlog 9. Decisiones del autor: un **murciélago** que, al acercarte a una distancia configurable, vaya hacia ti **bajando más de lo normal** para facilitar pisarlo; **de momento todos se pueden pisar** (quizá más adelante haya alguno que no); el escarabajo **cae por gravedad y sube y baja rampas**; y que se coloquen unos pocos en las salas. Sustituye la parte de ADR-0014 por la que el escarabajo giraba ante las rampas y no tenía gravedad.

## Decisión
- `Enemy` gana `kind` (`Walker`, `Bat`) y `detect_radius`. En Tiled, clase `bat` con esa propiedad (64 por defecto); en texto, `M`.
- **Escarabajo:** sigue el suelo bajo el centro de su cuerpo (hasta 4 px por encima y 2 por debajo), cae si no lo hay, y sigue dando la vuelta ante paredes, bordes, límites y pinchos.
- **Murciélago** (16×12): patrulla en ocho; ataca dentro del radio a 0,7 px/paso (menos de la mitad que corriendo) y lo deja a 1,5 radios; **apunta a la altura de los pies del jugador y no le sigue cuando salta**; tras morder se retira 1 s; no entra en la roca; derrotado, cae al suelo.
- Dibujo propio con ojos rojos al atacar.
- Cuatro murciélagos en las salas 03, 07, 08 y 09, sin tocar el escenario, en huecos abiertos sobre un suelo ancho y sin pinchos, lejos de entradas y checkpoints. (Los primeros cinco, en las salas 02, 04, 05, 07 y 08, complicaban demasiado el juego.)

## Alternativas descartadas
- **Que persiga a la altura del jugador siempre:** no se podría pisar.
- **Búsqueda de caminos para rodear paredes:** mucho más complejo para salas pequeñas.
- **Más tipos** (perseguidor de suelo, saltador, araña): no los pidió.

## Consecuencias
- `level_check` sigue sin modelar enemigos: la dificultad se comprueba jugando.
- Un enemigo que no se pueda matar necesitará una propiedad más y cambiar la regla de contacto.
