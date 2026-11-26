---
title: Pruebas
section: trabajar
order: 1
summary: Cómo se prueba un juego sin abrir una ventana.
---

```bash
ctest --test-dir build --output-on-failure   # todo
./build/tests                                # solo las pruebas unitarias (rápidas)
```

Se usa **doctest**. Las pruebas **nunca enlazan SDL**: comprueban el dominio, la aplicación, la carga de mapas,
las herramientas y la presentación pura. Además, una prueba lanza `level_check` sobre las salas reales.

## Cómo es una prueba

Montar una situación, hacer algo y comprobar:

<!-- src: game/tests/exit_test.cpp -->
```cpp
TEST_CASE("an exit that is not touched does not win") {
    GameSession session(makeWorld(), "only", 200.0F, test_maps::kFloorTop - 16.0F);
    for (int step = 0; step < 120; ++step) {
        session.update(InputState{});
    }
    CHECK_FALSE(session.won());
}
```

- `TEST_CASE` da nombre a la prueba, en forma de frase.
- `CHECK` anota el fallo y sigue; `REQUIRE` detiene la prueba.
- `test_maps::makeMap` crea una sala con suelo, para no repetirlo.

## Pruebas sobre los niveles reales

Algunas cargan los mapas del juego y comprueban reglas de diseño ("los tres orbes flotan y están en salas
distintas"). Si rompes una al editar un nivel, la prueba te lo dice.

## Lo que no cubren

El renderer, el teclado y el audio se comprueban **jugando**. Y que algo pase las pruebas no significa que se
sienta bien.

En GitHub, cada *pull request* compila y pasa las pruebas automáticamente (`.github/workflows/ci.yml`).
