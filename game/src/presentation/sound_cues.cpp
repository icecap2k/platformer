#include "presentation/sound_cues.h"

#include <variant>

namespace presentation {

namespace {

// Lets std::visit take a set of lambdas, one per alternative.
template <class... Handlers>
struct Overloaded : Handlers... {
    using Handlers::operator()...;
};
template <class... Handlers>
Overloaded(Handlers...) -> Overloaded<Handlers...>;

}  // namespace

std::optional<Sound> soundFor(const application::GameEvent& event) {
    using namespace application;
    return std::visit(
        Overloaded{
            [](const RoomChanged&) -> std::optional<Sound> { return std::nullopt; },
            [](const CoinCollected&) -> std::optional<Sound> { return Sound::Coin; },
            [](const FruitEaten&) -> std::optional<Sound> { return Sound::Fruit; },
            [](const PlayerJumped&) -> std::optional<Sound> { return Sound::Jump; },
            [](const PlayerLanded&) -> std::optional<Sound> { return Sound::Land; },
            [](const SlideStarted&) -> std::optional<Sound> { return Sound::Slide; },
            [](const PlayerHurt&) -> std::optional<Sound> { return Sound::Hurt; },
            [](const StalactiteWarned&) -> std::optional<Sound> { return Sound::StalactiteWarning; },
            [](const StalactiteFell&) -> std::optional<Sound> { return Sound::StalactiteCrack; },
            [](const StalactiteShattered&) -> std::optional<Sound> { return Sound::StalactiteCrash; },
            [](const EnemyStomped&) -> std::optional<Sound> { return Sound::Stomp; },
            [](const CheckpointReached&) -> std::optional<Sound> { return Sound::Checkpoint; },
            [](const PlayerRespawned&) -> std::optional<Sound> { return Sound::Restart; },
            [](const GameRestarted&) -> std::optional<Sound> { return Sound::Restart; },
        },
        event);
}

std::string soundName(Sound sound) {
    switch (sound) {
        case Sound::Jump:
            return "jump";
        case Sound::Land:
            return "land";
        case Sound::Coin:
            return "coin";
        case Sound::Fruit:
            return "fruit";
        case Sound::Hurt:
            return "hurt";
        case Sound::Slide:
            return "slide";
        case Sound::StalactiteWarning:
            return "stalactite_warning";
        case Sound::StalactiteCrack:
            return "stalactite_crack";
        case Sound::StalactiteCrash:
            return "stalactite_crash";
        case Sound::Checkpoint:
            return "checkpoint";
        case Sound::Stomp:
            return "stomp";
        case Sound::Restart:
            return "restart";
    }
    return "";
}

std::vector<Sound> allSounds() {
    return {Sound::Jump,           Sound::Land,           Sound::Coin,
            Sound::Fruit,          Sound::Hurt,           Sound::Slide,
            Sound::StalactiteWarning, Sound::StalactiteCrack, Sound::StalactiteCrash,
            Sound::Stomp,      Sound::Checkpoint, Sound::Restart};
}

}  // namespace presentation
