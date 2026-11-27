# ADR-0030: Jugar con mando, a la vez que con teclado

- Estado: Aceptada
- Fecha: 2026-11-27

## Contexto
Backlog 18: jugar con un controlador, decidiendo cómo convive con el teclado. Amplía ADR-0016 (menús).

## Decisión
- **Los dos a la vez:** una acción está activa si cualquiera la pide (`combine`); las pulsaciones de menú de ambos se juntan.
- `GamepadInputMapper` (SDL) lee los mandos y rellena un `GamepadSnapshot`; `gamepad_mapping` (librería sin SDL, `infrastructure_input`) decide qué significa. El dominio y la aplicación no cambian.
- Se pueden conectar y desconectar en cualquier momento; varios mandos se leen juntos. Desconectar el último en plena partida **pausa**.
- **Botones:** sur salta; cruceta y stick izquierdo mueven y agachan (zonas muertas 0,35 y 0,65); `Start` pausa; este vuelve en los menús y **no hace nada jugando**; `Select` silencia; los gatillos superiores cambian la música.
- Las ayudas en pantalla nombran los botones cuando hay un mando.

## Alternativas descartadas
- **Elegir un dispositivo en un menú**, o cambiar según el último usado: más estado sin ventaja.
- **Saltar con arriba**, como en el teclado: en un mando provoca saltos al correr.
- **Eventos de SDL** para el mando: obligaría a repartir la cola entre dos adaptadores.

## Consecuencias
- Sin remapeo de botones (backlog 25) ni vibración.
- Una pulsación más corta que un fotograma podría perderse.
- Probado con un **mando virtual** de SDL; con mandos físicos se comprueba jugando.
