#pragma once

#include <optional>
#include <string>
#include <vector>

#include "application/game_event.h"

namespace presentation {

// The sound effects of the game. Sound is a presentation concern: the game logic only
// reports what happened (GameEvent) and this decides which sound, if any, goes with it,
// so the rules never know about audio. Pure logic (no SDL), so it is unit tested; the
// files are made by tools/audio/make_sounds.py and played by infrastructure::AudioPlayer.
enum class Sound {
    Jump,
    Land,
    Coin,
    Fruit,
    Hurt,
    Slide,
    StalactiteWarning,  // the rumble before it comes loose
    StalactiteCrack,    // it comes loose
    StalactiteCrash,    // it shatters
    Checkpoint,         // a checkpoint lights up
    Stomp,              // the player lands on an enemy
    Orb,                // one of the orbs is taken
    Gate,               // the portcullis of the exit rises
    Win,                // the exit is reached
    Restart,            // all hearts lost: the game starts over
};

// The sound that goes with an event, or std::nullopt if it is silent (a room change).
// This is a std::visit, so adding an event without deciding its sound does not compile.
std::optional<Sound> soundFor(const application::GameEvent& event);

// File name of a sound without extension: assets/sounds/<name>.wav.
std::string soundName(Sound sound);

// Every sound, in a stable order (what the audio player has to load).
std::vector<Sound> allSounds();

}  // namespace presentation
