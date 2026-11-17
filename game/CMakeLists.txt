cmake_minimum_required(VERSION 3.20)
project(platformer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PLATFORMER_BUILD_TESTS "Build the unit tests" ON)

set(WARNINGS -Wall -Wextra -Wpedantic)

# --- Domain: pure C++, NO SDL. Game rules live here and are unit tested. ---
add_library(domain STATIC
    src/domain/version.cpp
    src/domain/time/fixed_timestep.cpp
    src/domain/physics/player_physics.cpp
    src/domain/world/tile_map.cpp
    src/domain/world/room.cpp
    src/domain/world/room_graph.cpp
    src/domain/world/coin_collection.cpp
    src/domain/rules/room_transition.cpp
    src/domain/rules/coin_pickup.cpp)
target_include_directories(domain PUBLIC src)
target_compile_options(domain PRIVATE ${WARNINGS})

# --- Application: orchestrates the domain. Still no SDL. ---
add_library(application STATIC
    src/application/game_session.cpp)
target_link_libraries(application PUBLIC domain)
target_compile_options(application PRIVATE ${WARNINGS})

# --- Asset loading: reads files into domain objects. No SDL, so it is testable. ---
add_library(infrastructure_assets STATIC
    src/infrastructure/assets/tiled_map_loader.cpp)
target_include_directories(infrastructure_assets PUBLIC src)
# Third-party headers are marked SYSTEM so their warnings don't pollute ours.
target_include_directories(infrastructure_assets SYSTEM PRIVATE third_party)
target_link_libraries(infrastructure_assets PUBLIC domain)
target_compile_options(infrastructure_assets PRIVATE ${WARNINGS})

# --- Infrastructure and presentation: the layers allowed to depend on SDL3. ---
find_package(SDL3 REQUIRED)

add_library(infrastructure STATIC
    src/infrastructure/sdl/window.cpp
    src/infrastructure/input/keyboard_input_mapper.cpp)
target_link_libraries(infrastructure PUBLIC domain infrastructure_assets SDL3::SDL3)
target_compile_options(infrastructure PRIVATE ${WARNINGS})

add_library(presentation STATIC
    src/presentation/game_renderer.cpp)
target_link_libraries(presentation PUBLIC application SDL3::SDL3)
target_compile_options(presentation PRIVATE ${WARNINGS})

# --- Executable: composition root. ---
add_executable(platformer src/main.cpp)
target_link_libraries(platformer PRIVATE infrastructure presentation application)  # domain comes transitively
target_compile_options(platformer PRIVATE ${WARNINGS})

# Copy the assets next to the executable so it finds them regardless of the
# directory it is launched from.
add_custom_command(TARGET platformer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:platformer>/assets)

# --- Tests: never link SDL. ---
if(PLATFORMER_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG v2.4.12)
    FetchContent_MakeAvailable(doctest)

    enable_testing()
    add_executable(tests
        tests/main.cpp
        tests/version_test.cpp
        tests/fixed_timestep_test.cpp
        tests/input_state_test.cpp
        tests/tile_map_test.cpp
        tests/player_physics_test.cpp
        tests/room_transition_test.cpp
        tests/coin_test.cpp
        tests/game_session_test.cpp
        tests/tiled_map_loader_test.cpp)
    target_link_libraries(tests PRIVATE application infrastructure_assets domain doctest::doctest)
    target_compile_definitions(tests PRIVATE
        PLATFORMER_ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_test(NAME tests COMMAND tests)
endif()
