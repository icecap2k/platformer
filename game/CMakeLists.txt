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
    src/domain/world/stalactite_traps.cpp
    src/domain/rules/room_transition.cpp
    src/domain/rules/coin_pickup.cpp
    src/domain/rules/damage.cpp)
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
    src/infrastructure/sdl/texture.cpp
    src/infrastructure/input/keyboard_input_mapper.cpp)
target_include_directories(infrastructure SYSTEM PRIVATE third_party)
target_link_libraries(infrastructure PUBLIC domain infrastructure_assets SDL3::SDL3)
target_compile_options(infrastructure PRIVATE ${WARNINGS})

# Animation rules are pure logic (no SDL), so they get their own library and
# the tests can use them.
add_library(presentation_animation STATIC
    src/presentation/animation.cpp
    src/presentation/decoration.cpp
    src/presentation/autotile.cpp
    src/presentation/drip.cpp)
target_link_libraries(presentation_animation PUBLIC domain)
target_compile_options(presentation_animation PRIVATE ${WARNINGS})

add_library(presentation STATIC
    src/presentation/game_renderer.cpp)
target_link_libraries(presentation PUBLIC application presentation_animation infrastructure SDL3::SDL3)
target_compile_options(presentation PRIVATE ${WARNINGS})

# --- Level tools: reuse the real game physics to check and build levels. ---
# They depend only on the domain and the asset loader (no SDL).
add_library(level_tools STATIC
    tools/level_tools/reachability.cpp
    tools/level_tools/text_level.cpp)
target_include_directories(level_tools PUBLIC tools)
target_include_directories(level_tools SYSTEM PRIVATE third_party)
target_link_libraries(level_tools PUBLIC domain)
target_compile_options(level_tools PRIVATE ${WARNINGS})

foreach(tool level_check level_from_text level_to_text)
    add_executable(${tool} tools/${tool}.cpp)
    target_link_libraries(${tool} PRIVATE level_tools infrastructure_assets)
    target_compile_options(${tool} PRIVATE ${WARNINGS})
endforeach()

# --- Executable: composition root. ---
add_executable(platformer src/main.cpp)
target_link_libraries(platformer PRIVATE presentation)  # everything else comes transitively
target_compile_options(platformer PRIVATE ${WARNINGS})

# Copy the assets next to the executable so it finds them regardless of the
# directory it is launched from. A custom target (not POST_BUILD of the
# executable) so that changing only an image or a map is picked up on the next
# build, without needing to recompile any code.
add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:platformer>/assets)
add_dependencies(platformer copy_assets)

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
        tests/damage_test.cpp
        tests/stalactite_test.cpp
        tests/slope_test.cpp
        tests/crouch_test.cpp
        tests/animation_test.cpp
        tests/decoration_test.cpp
        tests/autotile_test.cpp
        tests/drip_test.cpp
        tests/game_session_test.cpp
        tests/tiled_map_loader_test.cpp
        tests/level_tools_test.cpp)
    target_link_libraries(tests PRIVATE application infrastructure_assets presentation_animation
                          level_tools domain doctest::doctest)
    target_compile_definitions(tests PRIVATE
        PLATFORMER_ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
        PLATFORMER_LEVELS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/levels")
    add_test(NAME tests COMMAND tests)

    # Proves that every shipped room and coin can be reached with the real physics.
    add_test(NAME shipped_levels_reachable
             COMMAND level_check ${CMAKE_CURRENT_SOURCE_DIR}/assets/tiled room_01)
endif()
