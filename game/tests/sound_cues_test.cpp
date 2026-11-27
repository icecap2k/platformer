#include <doctest/doctest.h>

#include <set>
#include <string>

#include "presentation/sound_cues.h"

using namespace presentation;
using namespace application;

TEST_CASE("soundFor maps every event to its sound") {
    CHECK(soundFor(GameEvent{PlayerJumped{}}) == Sound::Jump);
    CHECK(soundFor(GameEvent{PlayerLanded{3.0F}}) == Sound::Land);
    CHECK(soundFor(GameEvent{CoinCollected{"r", 0, 1}}) == Sound::Coin);
    CHECK(soundFor(GameEvent{FruitEaten{"r", 0, 3, 1}}) == Sound::Fruit);
    CHECK(soundFor(GameEvent{PlayerHurt{"r", 2}}) == Sound::Hurt);
    CHECK(soundFor(GameEvent{SlideStarted{}}) == Sound::Slide);
    CHECK(soundFor(GameEvent{StalactiteWarned{"r", 0}}) == Sound::StalactiteWarning);
    CHECK(soundFor(GameEvent{StalactiteFell{"r", 0}}) == Sound::StalactiteCrack);
    CHECK(soundFor(GameEvent{StalactiteShattered{"r", 0}}) == Sound::StalactiteCrash);
    CHECK(soundFor(GameEvent{EnemyStomped{"r", 0}}) == Sound::Stomp);
    CHECK(soundFor(GameEvent{CheckpointReached{"r", 0}}) == Sound::Checkpoint);
    CHECK(soundFor(GameEvent{PlayerRespawned{"r"}}) == Sound::Restart);
    CHECK(soundFor(GameEvent{GameWon{5, 12}}) == Sound::Win);
    CHECK(soundFor(GameEvent{GameRestarted{}}) == Sound::Restart);

    SUBCASE("changing room is silent") {
        CHECK_FALSE(soundFor(GameEvent{RoomChanged{"a", "b"}}).has_value());
    }

    SUBCASE("a wasted fruit still gets the fruit sound") {
        CHECK(soundFor(GameEvent{FruitEaten{"r", 0, 3, 0}}) == Sound::Fruit);
    }
}

TEST_CASE("every sound has a distinct file name") {
    std::set<std::string> names;
    for (const Sound sound : allSounds()) {
        const std::string name = soundName(sound);
        CHECK_FALSE(name.empty());
        CHECK(name.find('.') == std::string::npos);  // no extension: the player adds it
        CHECK(name.find(' ') == std::string::npos);
        names.insert(name);
    }
    CHECK(names.size() == allSounds().size());
}

TEST_CASE("allSounds lists every sound once") {
    CHECK(allSounds().size() == 16);
    // Every sound an event can produce is in the list (so the player has loaded it).
    const std::vector<GameEvent> events = {PlayerJumped{},        PlayerLanded{2.0F},       CoinCollected{"r", 0, 1},
                                           FruitEaten{"r", 0, 3, 1}, PlayerHurt{"r", 1},     SlideStarted{},
                                           StalactiteWarned{"r", 0}, StalactiteFell{"r", 0}, StalactiteShattered{"r", 0},
                                           EnemyStomped{"r", 0}, CheckpointReached{"r", 0},
                                           PlayerRespawned{"r"}, GameWon{1, 2}, GameRestarted{}};
    const auto sounds = allSounds();
    for (const GameEvent& event : events) {
        const auto sound = soundFor(event);
        REQUIRE(sound.has_value());
        bool listed = false;
        for (const Sound candidate : sounds) {
            listed = listed || candidate == *sound;
        }
        CHECK(listed);
    }
}
