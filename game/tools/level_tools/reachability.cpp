#include "level_tools/reachability.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <stdexcept>
#include <unordered_set>

#include "domain/entities/player.h"
#include "domain/input/input_state.h"
#include "domain/physics/player_physics.h"
#include "domain/rules/room_transition.h"

namespace level_tools {

namespace {

struct State {
    std::size_t room;
    domain::Player player;
};

// Packs a state into one integer. The player's position and speed are
// rounded (0.5 px, 0.1 px, 0.1 px/step) and the jump state is included, which makes the state space finite.
// The offsets keep every field non-negative so fields cannot overlap.
std::uint64_t keyOf(const State& state) {
    const auto x = static_cast<std::uint64_t>(std::llround(state.player.x * 2.0F) + 8192);
    const auto y = static_cast<std::uint64_t>(std::llround(state.player.y * 10.0F) + 8192);
    const auto vy = static_cast<std::uint64_t>(std::llround(state.player.velocity_y * 10.0F) + 256);
    // Jump state, reduced to what can still change the future (fewer distinct
    // states = a much smaller search): on the ground the grace period is always
    // full, and without it (airborne, no coyote time left) holding the button
    // no longer matters.
    const bool grounded = state.player.on_ground;
    const int coyote_steps = grounded ? domain::kCoyoteSteps : state.player.coyote_steps;
    const bool held = (grounded || coyote_steps > 0) && state.player.jump_held;
    const auto coyote = static_cast<std::uint64_t>(coyote_steps);  // 0..kCoyoteSteps (3 bits)
    const std::uint64_t flags = (coyote << 2) | (held ? 2U : 0U) |
                                (state.player.on_ground ? 1U : 0U);
    return (static_cast<std::uint64_t>(state.room) << 48) | (x << 32) | (y << 16) | (vy << 5) | flags;
}

std::vector<domain::InputState> allInputs() {
    std::vector<domain::InputState> inputs;
    for (int direction : {-1, 0, 1}) {
        for (bool jump : {false, true}) {
            domain::InputState input;
            input.left = direction < 0;
            input.right = direction > 0;
            input.jump = jump;
            inputs.push_back(input);
        }
    }
    return inputs;
}

}  // namespace

bool ReachabilityReport::allOk() const {
    for (const auto& [id, reachable] : room_reachable) {
        if (!reachable) {
            return false;
        }
    }
    return std::all_of(coins.begin(), coins.end(),
                       [](const CoinStatus& coin) { return coin.touchable; });
}

ReachabilityReport analyzeReachability(const domain::RoomGraph& world,
                                       const ReachabilityOptions& options) {
    const std::vector<domain::RoomId> ids = world.ids();
    const auto start_it = std::find(ids.begin(), ids.end(), options.start_room);
    if (start_it == ids.end()) {
        throw std::invalid_argument("Unknown start room: " + options.start_room);
    }

    ReachabilityReport report;
    for (const domain::RoomId& id : ids) {
        report.room_reachable[id] = false;
        report.room_standable[id] = false;
        const auto& coins = world.room(id).coins();
        for (std::size_t index = 0; index < coins.size(); ++index) {
            report.coins.push_back(CoinStatus{id, index, coins[index], false});
        }
    }

    auto indexOf = [&ids](const domain::RoomId& id) {
        return static_cast<std::size_t>(std::find(ids.begin(), ids.end(), id) - ids.begin());
    };

    State start;
    start.room = static_cast<std::size_t>(start_it - ids.begin());
    start.player.x = options.spawn_x;
    start.player.y = options.spawn_y;

    const std::vector<domain::InputState> inputs = allInputs();
    std::unordered_set<std::uint64_t> seen{keyOf(start)};
    std::deque<State> queue{start};

    while (!queue.empty()) {
        if (seen.size() >= options.max_states) {
            report.truncated = true;
            break;
        }

        const State current = queue.front();
        queue.pop_front();

        const domain::Room& room = world.room(ids[current.room]);
        report.room_reachable[room.id()] = true;
        if (current.player.on_ground) {
            report.room_standable[room.id()] = true;
        }
        for (CoinStatus& coin : report.coins) {
            if (coin.room == room.id() && !coin.touchable &&
                current.player.bounds().overlaps(coin.coin.bounds())) {
                coin.touchable = true;
            }
        }

        for (const domain::InputState& input : inputs) {
            State next = current;
            domain::stepPlayer(next.player, input, room.map());
            domain::confineToMap(next.player, room.map(), domain::openEdgesOf(room));
            if (const auto side = domain::exitedThrough(next.player, room.map())) {
                const domain::RoomId target = *room.neighbor(*side);  // a copy: neighbor() returns by value
                next.room = indexOf(target);
                domain::enterRoom(next.player, *side, world.room(target).map());
            }
            if (seen.insert(keyOf(next)).second) {
                queue.push_back(next);
            }
        }
    }

    report.states_explored = seen.size();
    return report;
}

}  // namespace level_tools
