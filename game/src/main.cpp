// Composition root: the only place that knows every layer and wires them together.
#include <SDL3/SDL.h>

#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "application/game_flow.h"
#include "application/game_session.h"
#include "domain/time/fixed_timestep.h"
#include "infrastructure/assets/tiled_map_loader.h"
#include "infrastructure/audio/audio_player.h"
#include "infrastructure/input/gamepad_input_mapper.h"
#include "infrastructure/input/keyboard_input_mapper.h"
#include "infrastructure/sdl/window.h"
#include "presentation/game_renderer.h"
#include "presentation/music_volume.h"
#include "presentation/sound_cues.h"

namespace {

constexpr int kLogicalWidth = 320;
constexpr int kLogicalHeight = 240;
constexpr int kWindowScale = 3;
constexpr std::uint64_t kStepNs = 1'000'000'000ULL / 60;  // 60 simulation steps per second

// How loud things sound next to the music (1.0 = as synthesised).
constexpr float kEffectsGain = 0.6F;
constexpr float kDripGain = 0.28F;  // the drops are scenery: quieter than what the player does

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
    } else if (const auto* reached = std::get_if<application::CheckpointReached>(&event)) {
        std::cout << "[event] checkpoint reached in " << reached->room << std::endl;
    } else if (const auto* respawned = std::get_if<application::PlayerRespawned>(&event)) {
        std::cout << "[event] no hearts left: back at the last checkpoint in " << respawned->room << std::endl;
    } else if (const auto* won = std::get_if<application::GameWon>(&event)) {
        std::cout << "[event] the exit! " << won->coins << "/" << won->total_coins << " coins" << std::endl;
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
    for (const char* name : {"room_01", "room_02", "room_03", "room_04", "room_05", "room_06", "room_07",
                              "room_08", "room_09"}) {
        rooms.push_back(infrastructure::loadTiledRoom(assetPath(std::string("tiled/") + name + ".tmj")));
    }
    return domain::RoomGraph(std::move(rooms));
}

}  // namespace

int main() {
    try {
        infrastructure::Window window("platformer", kLogicalWidth, kLogicalHeight, kWindowScale);
        infrastructure::KeyboardInputMapper input_mapper;
        infrastructure::GamepadInputMapper gamepad;  // after the window, so it is closed first
        presentation::GameRenderer renderer(window.renderer(), assetPath(""));
        domain::FixedTimestep timestep(kStepNs);
        application::GameFlow flow;
        std::optional<application::GameSession> session;  // created when the player picks Start
        // Sound: after the window so it is closed first; it stays silent if there is no audio.
        infrastructure::AudioPlayer audio(assetPath("sounds/"), soundNames());
        std::cout << "[audio] " << audio.loadedCount() << " sounds loaded"
                  << (audio.available() ? ", press M to mute, - and + for the music" : " (no audio device)")
                  << std::endl;
        presentation::MusicVolume music;
        for (const presentation::Sound sound : presentation::allSounds()) {
            const float gain = sound == presentation::Sound::Drip ? kDripGain : kEffectsGain;
            audio.setGain(presentation::soundName(sound), gain);
        }
        audio.startMusic(assetPath("music/cave.wav"));

        std::uint64_t previous_ns = SDL_GetTicksNS();

        while (!input_mapper.quitRequested() && !flow.quitRequested()) {
            input_mapper.poll();
            gamepad.update();  // the poll above pumped the events the pads are read from
            if (const auto connected = gamepad.connectionChange()) {
                renderer.setGamepad(*connected);
                renderer.showNotice(*connected ? "GAMEPAD CONNECTED" : "GAMEPAD DISCONNECTED");
                if (!*connected && flow.screen() == application::Screen::Playing) {
                    flow.handle(domain::MenuCommand::Pause);  // nobody left holding the controls
                }
            }
            // Both devices work at any time.
            const auto pad = gamepad.actions(flow.screen() == application::Screen::Playing);
            const bool keyboard_mute = input_mapper.takeMuteToggle();
            if (keyboard_mute || pad.toggle_mute) {
                audio.toggleMute();
                std::cout << "[audio] " << (audio.muted() ? "muted" : "sound on") << std::endl;
                renderer.showNotice(audio.muted() ? "SOUND OFF" : "SOUND ON");
            }
            if (const int change = input_mapper.takeMusicSteps() + pad.music_steps; change != 0) {
                music.change(change);
                renderer.showNotice(music.label());
            }
            std::vector<domain::MenuCommand> commands = input_mapper.takeMenuCommands();
            commands.insert(commands.end(), pad.menu.begin(), pad.menu.end());
            for (const domain::MenuCommand command : commands) {
                if (flow.handle(command)) {
                    session.emplace(loadWorld(), "room_01", kSpawnX, kSpawnY);  // a fresh game
                }
            }
            const domain::InputState input = infrastructure::combine(input_mapper.state(), gamepad.state());

            const std::uint64_t now_ns = SDL_GetTicksNS();
            const int steps = timestep.advance(now_ns - previous_ns);
            previous_ns = now_ns;

            // While paused (or on the title) the clock still runs, but no step is taken.
            // Nor while the picture is moving to another room or screen.
            const bool playing = flow.screen() == application::Screen::Playing && session && !renderer.inTransition();
            const int game_steps = playing ? steps : 0;
            for (int i = 0; i < game_steps; ++i) {
                for (const application::GameEvent& event : session->update(input)) {
                    report(event);
                    renderer.onEvent(event);
                    if (std::holds_alternative<application::GameWon>(event)) {
                        flow.win();
                    }
                    if (const auto sound = presentation::soundFor(event)) {
                        audio.play(presentation::soundName(*sound));
                    }
                }
                if (renderer.inTransition()) {
                    break;  // the rest of this frame's steps would happen off screen
                }
            }

            renderer.draw(session ? &*session : nullptr, flow);
            if (renderer.takeDripSplash()) {
                audio.play(presentation::soundName(presentation::Sound::Drip));
            }
            audio.setMusicGain(music.gain(flow.screen() == application::Screen::Paused));
            audio.update();
        }
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
