// Composition root: the only place that knows every layer and wires them together.
#include <SDL3/SDL.h>

#include <exception>
#include <iostream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "application/game_session.h"
#include "domain/time/fixed_timestep.h"
#include "infrastructure/assets/tiled_map_loader.h"
#include "infrastructure/audio/audio_player.h"
#include "infrastructure/input/keyboard_input_mapper.h"
#include "infrastructure/sdl/window.h"
#include "presentation/game_renderer.h"
#include "presentation/sound_cues.h"

namespace {

constexpr int kLogicalWidth = 320;
constexpr int kLogicalHeight = 240;
constexpr int kWindowScale = 3;
constexpr std::uint64_t kStepNs = 1'000'000'000ULL / 60;  // 60 simulation steps per second

constexpr float kSpawnX = 40.0F;
constexpr float kSpawnY = 60.0F;  // in mid-air, so the first thing you see is the fall

// The build copies `assets/` next to the executable.
std::string assetPath(const std::string& relative) {
    const char* base = SDL_GetBasePath();
    return std::string(base != nullptr ? base : "") + "assets/" + relative;
}

// Reacts to what happened in the game. Today it only logs; later this is the
// place to play sounds or spawn effects, without touching the game logic.
void report(const application::GameEvent& event) {
    if (const auto* changed = std::get_if<application::RoomChanged>(&event)) {
        std::cout << "[event] room " << changed->from << " -> " << changed->to << '\n';
    } else if (const auto* fruit = std::get_if<application::FruitEaten>(&event)) {
        std::cout << "[event] fruit eaten in " << fruit->room << " (" << fruit->health_after
                  << " hearts" << (fruit->hearts_gained == 0 ? ", wasted" : "") << ")" << std::endl;
    } else if (const auto* hurt = std::get_if<application::PlayerHurt>(&event)) {
        std::cout << "[event] player hurt in " << hurt->room << " (" << hurt->health_left
                  << " hearts left)" << std::endl;
    } else if (const auto* fell = std::get_if<application::StalactiteFell>(&event)) {
        std::cout << "[event] a stalactite trap came loose in " << fell->room << std::endl;
    } else if (const auto* stomped = std::get_if<application::EnemyStomped>(&event)) {
        std::cout << "[event] enemy stomped in " << stomped->room << std::endl;
    } else if (std::holds_alternative<application::GameRestarted>(event)) {
        std::cout << "[event] game over: starting again from the first room" << std::endl;
    } else if (const auto* coin = std::get_if<application::CoinCollected>(&event)) {
        std::cout << "[event] coin collected in " << coin->room << " (" << coin->total_collected
                  << " so far)" << std::endl;
    }
}

// The names of every sound file the audio player has to load.
std::vector<std::string> soundNames() {
    std::vector<std::string> names;
    for (const presentation::Sound sound : presentation::allSounds()) {
        names.push_back(presentation::soundName(sound));
    }
    return names;
}

domain::RoomGraph loadWorld() {
    std::vector<domain::Room> rooms;
    for (const char* name : {"room_01", "room_02", "room_03", "room_04"}) {
        rooms.push_back(infrastructure::loadTiledRoom(assetPath(std::string("tiled/") + name + ".tmj")));
    }
    return domain::RoomGraph(std::move(rooms));
}

}  // namespace

int main() {
    try {
        infrastructure::Window window("platformer", kLogicalWidth, kLogicalHeight, kWindowScale);
        infrastructure::KeyboardInputMapper input_mapper;
        presentation::GameRenderer renderer(window.renderer(), assetPath(""));
        domain::FixedTimestep timestep(kStepNs);
        application::GameSession session(loadWorld(), "room_01", kSpawnX, kSpawnY);
        // Sound: after the window so it is closed first; it stays silent if there is no audio.
        infrastructure::AudioPlayer audio(assetPath("sounds/"), soundNames());
        std::cout << "[audio] " << audio.loadedCount() << " sounds loaded"
                  << (audio.available() ? ", press M to mute" : " (no audio device)") << std::endl;

        std::uint64_t previous_ns = SDL_GetTicksNS();

        while (!input_mapper.quitRequested()) {
            input_mapper.poll();
            if (input_mapper.takeMuteToggle()) {
                audio.toggleMute();
                std::cout << "[audio] " << (audio.muted() ? "muted" : "sound on") << std::endl;
            }
            const domain::InputState input = input_mapper.state();

            const std::uint64_t now_ns = SDL_GetTicksNS();
            const int steps = timestep.advance(now_ns - previous_ns);
            previous_ns = now_ns;

            for (int i = 0; i < steps; ++i) {
                for (const application::GameEvent& event : session.update(input)) {
                    report(event);
                    if (const auto sound = presentation::soundFor(event)) {
                        audio.play(presentation::soundName(*sound));
                    }
                }
            }

            renderer.draw(session);
        }
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
