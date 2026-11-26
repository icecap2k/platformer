#include "level_tools/reachability.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <stdexcept>

#include "domain/entities/player.h"
#include "domain/input/input_state.h"
#include "domain/physics/player_physics.h"
#include "domain/rules/damage.h"
#include "domain/rules/room_transition.h"
#include "level_tools/state_set.h"

namespace level_tools {

namespace {

struct State {
    std::size_t room;
    domain::Player player;
};

// Packs a state into one integer. The player's position and speed are
// rounded (0.5 px, 0.1 px, 0.1 px/step) and the jump and slide state are included, which makes the state space finite.
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
    // Sliding down a ramp: the carried speed matters while it lasts (4 bits for
    // 0..kSlideCarrySteps), and its direction only then.
    const auto slide = static_cast<std::uint64_t>(state.player.slide_steps);  // 0..kSlideCarrySteps (4 bits)
    const std::uint64_t slide_direction = (state.player.slide_steps > 0 && state.player.slide_direction > 0) ? 1U : 0U;
    const std::uint64_t flags = (slide_direction << 9) | (slide << 5) | (coyote << 2) | (held ? 2U : 0U) |
                                (state.player.on_ground ? 1U : 0U);
    // Layout, from the low bits: flags 10, vy 11, y 16, x 16, room 11.
    return (static_cast<std::uint64_t>(state.room) << 53) | (x << 37) | (y << 21) | (vy << 10) | flags;
}

// The search never presses `down`, so it does not model crouching or sliding.
// That can only under-estimate what the player can do (a smaller box and extra
// speed would only help), so a route it finds is real; a level that REQUIRES a
// crouch (a low passage) would be reported as unreachable until the search learns
// to crouch. See ADR-0011.
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

// What the search needs of a room, looked up once instead of on every step.
struct RoomInfo {
    const domain::Room* room = nullptr;
    domain::OpenEdges open;
    std::size_t neighbor[4] = {0, 0, 0, 0};  // index of the room behind each Side, if any
    std::vector<std::size_t> coins;          // entries of the report still to be touched
    std::vector<std::size_t> fruits;
    std::vector<std::size_t> checkpoints;
    std::vector<std::size_t> orbs;
    std::vector<std::size_t> exits;
    bool reached = false;
    bool stood = false;
};

// Marks as touched the pending items the player overlaps and forgets about them.
// Returns how many there were.
template <class Status, class Bounds>
std::size_t touch(std::vector<std::size_t>& pending, std::vector<Status>& statuses, const domain::AABB& body,
                  Bounds bounds) {
    std::size_t touched = 0;
    for (std::size_t i = 0; i < pending.size();) {
        Status& status = statuses[pending[i]];
        if (body.overlaps(bounds(status))) {
            status.touchable = true;
            pending[i] = pending.back();
            pending.pop_back();
            ++touched;
        } else {
            ++i;
        }
    }
    return touched;
}

}  // namespace

bool ReachabilityReport::allOk() const {
    for (const auto& [id, reachable] : room_reachable) {
        if (!reachable) {
            return false;
        }
    }
    return std::all_of(coins.begin(), coins.end(),
                       [](const CoinStatus& coin) { return coin.touchable; }) &&
           std::all_of(fruits.begin(), fruits.end(),
                       [](const FruitStatus& fruit) { return fruit.touchable; }) &&
           std::all_of(checkpoints.begin(), checkpoints.end(),
                       [](const CheckpointStatus& checkpoint) { return checkpoint.touchable; }) &&
           std::all_of(orbs.begin(), orbs.end(), [](const OrbStatus& orb) { return orb.touchable; }) &&
           std::all_of(exits.begin(), exits.end(), [](const ExitStatus& exit) { return exit.touchable; });
}

ReachabilityReport analyzeReachability(const domain::RoomGraph& world,
                                       const ReachabilityOptions& options) {
    const std::vector<domain::RoomId> ids = world.ids();
    const auto start_it = std::find(ids.begin(), ids.end(), options.start_room);
    if (start_it == ids.end()) {
        throw std::invalid_argument("Unknown start room: " + options.start_room);
    }

    auto indexOf = [&ids](const domain::RoomId& id) {
        return static_cast<std::size_t>(std::find(ids.begin(), ids.end(), id) - ids.begin());
    };

    ReachabilityReport report;
    std::vector<RoomInfo> rooms(ids.size());
    std::size_t remaining = 2 * ids.size();  // enter and stand in every room, and touch everything
    for (std::size_t r = 0; r < ids.size(); ++r) {
        const domain::RoomId& id = ids[r];
        const domain::Room& room = world.room(id);
        RoomInfo& info = rooms[r];
        info.room = &room;
        info.open = domain::openEdgesOf(room);
        for (const auto& [side, neighbor] : room.neighbors()) {
            info.neighbor[static_cast<std::size_t>(side)] = indexOf(neighbor);
        }
        report.room_reachable[id] = false;
        report.room_standable[id] = false;
        // One report entry per item, and the same entry in the room's list of pending ones.
        const auto add = [&](const auto& items, auto& statuses, std::vector<std::size_t>& pending) {
            for (std::size_t index = 0; index < items.size(); ++index) {
                pending.push_back(statuses.size());
                statuses.push_back({id, index, items[index], false});
                ++remaining;
            }
        };
        add(room.coins(), report.coins, info.coins);
        add(room.fruits(), report.fruits, info.fruits);
        add(room.checkpoints(), report.checkpoints, info.checkpoints);
        add(room.orbs(), report.orbs, info.orbs);
        add(room.exits(), report.exits, info.exits);
    }

    State start;
    start.room = static_cast<std::size_t>(start_it - ids.begin());
    start.player.x = options.spawn_x;
    start.player.y = options.spawn_y;

    const std::vector<domain::InputState> inputs = allInputs();
    StateSet seen;
    seen.insert(keyOf(start));
    std::deque<State> queue{start};

    while (!queue.empty()) {
        if (options.stop_when_all_found && remaining == 0) {
            report.stopped_early = true;
            break;
        }
        if (seen.size() >= options.max_states) {
            report.truncated = true;
            break;
        }

        const State current = queue.front();
        queue.pop_front();

        RoomInfo& info = rooms[current.room];
        const domain::Room& room = *info.room;
        if (!info.reached) {
            info.reached = true;
            report.room_reachable[room.id()] = true;
            --remaining;
        }
        if (current.player.on_ground && !info.stood) {
            info.stood = true;
            report.room_standable[room.id()] = true;
            --remaining;
        }
        const domain::AABB body = current.player.bounds();
        remaining -= touch(info.coins, report.coins, body, [](const CoinStatus& s) { return s.coin.bounds(); });
        remaining -= touch(info.fruits, report.fruits, body, [](const FruitStatus& s) { return s.fruit.bounds(); });
        remaining -= touch(info.checkpoints, report.checkpoints, body,
                           [](const CheckpointStatus& s) { return s.checkpoint.bounds(); });
        remaining -= touch(info.orbs, report.orbs, body, [](const OrbStatus& s) { return s.orb.bounds(); });
        remaining -= touch(info.exits, report.exits, body, [](const ExitStatus& s) { return s.exit.bounds(); });

        for (const domain::InputState& input : inputs) {
            State next = current;
            domain::stepPlayer(next.player, input, room.map());
            // Jump buffering (ADR-0017) cannot reach anything new: a remembered press equals pressing
            // again on the landing step, which the search already tries. Forgetting it keeps the
            // state space as small as before (remembering it multiplied it past the search limit).
            next.player.jump_buffer_steps = 0;
            domain::confineToMap(next.player, room.map(), info.open);
            if (const auto side = domain::exitedThrough(next.player, room.map())) {
                next.room = info.neighbor[static_cast<std::size_t>(*side)];
                domain::enterRoom(next.player, *side, rooms[next.room].room->map());
            }
            // Touching a hazard hurts: a route that goes through spikes is not a
            // route. (Falling stalactites are dynamic and are not modelled.)
            if (domain::hazardTouching(next.player, *rooms[next.room].room).has_value()) {
                continue;
            }
            if (seen.insert(keyOf(next))) {
                queue.push_back(next);
            }
        }
    }

    report.states_explored = seen.size();
    return report;
}

}  // namespace level_tools
