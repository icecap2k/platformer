#include <doctest/doctest.h>

#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "application/game_session.h"
#include "domain/rules/damage.h"
#include "domain/world/stalactite_traps.h"
#include "infrastructure/assets/tiled_map_loader.h"
#include "test_maps.h"

using application::GameSession;
using domain::InputState;
using domain::Room;
using domain::RoomGraph;
using domain::Side;

namespace {

// Five rooms in a plus shape, all empty with a floor:
//
//          north
//   west   center   east
//          south
RoomGraph makePlusWorld() {
    using Links = std::map<Side, domain::RoomId>;
    std::vector<Room> rooms;
    rooms.emplace_back("center", test_maps::makeMap(),
                       Links{{Side::North, "north"}, {Side::South, "south"},
                             {Side::East, "east"}, {Side::West, "west"}});
    rooms.emplace_back("north", test_maps::makeMap(), Links{{Side::South, "center"}});
    rooms.emplace_back("south", test_maps::makeMap(), Links{{Side::North, "center"}});
    rooms.emplace_back("east", test_maps::makeMap(), Links{{Side::West, "center"}});
    rooms.emplace_back("west", test_maps::makeMap(), Links{{Side::East, "center"}});
    return RoomGraph(std::move(rooms));
}

void run(GameSession& session, const InputState& input, int steps) {
    for (int i = 0; i < steps; ++i) {
        session.update(input);
    }
}

// Runs until the session leaves `room_id` (or `max_steps` pass).
void runUntilLeaving(GameSession& session, const InputState& input, const std::string& room_id,
                     int max_steps = 600) {
    for (int i = 0; i < max_steps && session.currentRoom().id() == room_id; ++i) {
        session.update(input);
    }
}

}  // namespace

TEST_CASE("GameSession basics") {
    GameSession session(makePlusWorld(), "center", 40.0F, 60.0F);

    SUBCASE("starts in the requested room, in the air") {
        CHECK(session.currentRoom().id() == "center");
        CHECK(session.player().x == 40.0F);
        CHECK(session.player().y == 60.0F);
        CHECK_FALSE(session.player().on_ground);
    }

    SUBCASE("the player settles on the floor") {
        run(session, InputState{}, 200);
        CHECK(session.player().on_ground);
        CHECK(session.currentRoom().id() == "center");
    }

    SUBCASE("input drives the player") {
        InputState input;
        input.right = true;
        run(session, input, 20);
        CHECK(session.player().x > 40.0F);
    }

    SUBCASE("rejects an unknown start room") {
        CHECK_THROWS_AS(GameSession(makePlusWorld(), "nowhere", 0.0F, 0.0F), std::invalid_argument);
    }
}

TEST_CASE("GameSession changes room through any of the four edges") {
    SUBCASE("east") {
        GameSession session(makePlusWorld(), "center", 100.0F, 200.0F);
        InputState right;
        right.right = true;
        runUntilLeaving(session, right, "center");

        CHECK(session.currentRoom().id() == "east");
        CHECK(session.player().x < 20.0F);  // arrived by the west side
        CHECK(session.player().on_ground);
    }

    SUBCASE("west") {
        GameSession session(makePlusWorld(), "center", 100.0F, 200.0F);
        InputState left;
        left.left = true;
        runUntilLeaving(session, left, "center");

        CHECK(session.currentRoom().id() == "west");
        CHECK(session.player().x > 280.0F);  // arrived by the east side
    }

    SUBCASE("north") {
        GameSession session(makePlusWorld(), "center", 100.0F, -12.0F);  // already half out
        session.update(InputState{});

        CHECK(session.currentRoom().id() == "north");
        CHECK(session.player().y > 200.0F);  // arrived at the bottom
    }

    SUBCASE("south") {
        GameSession session(makePlusWorld(), "center", 100.0F, 236.0F);  // already half out
        session.update(InputState{});

        CHECK(session.currentRoom().id() == "south");
        CHECK(session.player().y < 20.0F);  // arrived at the top
    }
}

TEST_CASE("GameSession walls where there is no neighbour") {
    SUBCASE("cannot leave a room through an edge without a neighbour") {
        GameSession session(makePlusWorld(), "east", 100.0F, 200.0F);
        InputState right;
        right.right = true;
        run(session, right, 500);

        CHECK(session.currentRoom().id() == "east");
        CHECK(session.player().x == doctest::Approx(312.0F));
    }

    SUBCASE("can go back and forth between two rooms") {
        GameSession session(makePlusWorld(), "center", 100.0F, 200.0F);
        InputState right;
        right.right = true;
        runUntilLeaving(session, right, "center");
        REQUIRE(session.currentRoom().id() == "east");

        InputState left;
        left.left = true;
        runUntilLeaving(session, left, "east");
        CHECK(session.currentRoom().id() == "center");
    }
}

namespace {

// center (has a coin at the spawn point) <-> east (empty)
RoomGraph makeCoinWorld() {
    using Links = std::map<Side, domain::RoomId>;
    std::vector<Room> rooms;
    rooms.emplace_back("center", test_maps::makeMap(), Links{{Side::East, "east"}},
                       std::vector<domain::Coin>{domain::Coin{100.0F, 204.0F}});
    rooms.emplace_back("east", test_maps::makeMap(), Links{{Side::West, "center"}});
    return RoomGraph(std::move(rooms));
}

int countCoinEvents(const std::vector<application::GameEvent>& events) {
    int count = 0;
    for (const auto& event : events) {
        if (std::holds_alternative<application::CoinCollected>(event)) {
            ++count;
        }
    }
    return count;
}

}  // namespace

TEST_CASE("GameSession coins") {
    GameSession session(makeCoinWorld(), "center", 100.0F, 200.0F);  // overlapping the coin
    CHECK(session.totalCoins() == 1);
    CHECK(session.collectedCoins().count() == 0);

    SUBCASE("touching a coin collects it and reports an event") {
        const auto events = session.update(InputState{});

        REQUIRE(events.size() == 1);
        const auto* collected = std::get_if<application::CoinCollected>(&events[0]);
        REQUIRE(collected != nullptr);
        CHECK(collected->room == "center");
        CHECK(collected->coin_index == 0);
        CHECK(collected->total_collected == 1);
        CHECK(session.collectedCoins().count() == 1);
    }

    SUBCASE("the same coin is never collected twice") {
        session.update(InputState{});
        CHECK(countCoinEvents(session.update(InputState{})) == 0);
        CHECK(session.collectedCoins().count() == 1);
    }

    SUBCASE("a collected coin stays collected after leaving and coming back") {
        session.update(InputState{});
        REQUIRE(session.collectedCoins().count() == 1);

        InputState right;
        right.right = true;
        runUntilLeaving(session, right, "center");
        REQUIRE(session.currentRoom().id() == "east");

        int coin_events = 0;
        InputState left;
        left.left = true;
        for (int i = 0; i < 600; ++i) {
            coin_events += countCoinEvents(session.update(left));
        }

        CHECK(session.currentRoom().id() == "center");
        CHECK(coin_events == 0);
        CHECK(session.collectedCoins().count() == 1);
    }

    SUBCASE("changing room reports a RoomChanged event") {
        InputState right;
        right.right = true;

        bool saw_room_change = false;
        for (int i = 0; i < 600 && !saw_room_change; ++i) {
            for (const auto& event : session.update(right)) {
                if (const auto* changed = std::get_if<application::RoomChanged>(&event)) {
                    CHECK(changed->from == "center");
                    CHECK(changed->to == "east");
                    saw_room_change = true;
                }
            }
        }
        CHECK(saw_room_change);
    }
}

TEST_CASE("GameSession with the shipped rooms") {
    std::vector<Room> rooms;
    for (const char* name : {"room_01", "room_02", "room_03", "room_04"}) {
        rooms.push_back(infrastructure::loadTiledRoom(std::string(PLATFORMER_ASSETS_DIR) +
                                                      "/tiled/" + name + ".tmj"));
    }
    GameSession session(RoomGraph(std::move(rooms)), "room_01", 40.0F, 60.0F);

    SUBCASE("walking east from the start room climbs the ramp and reaches room_02, no jumping needed") {
        run(session, InputState{}, 120);  // land first
        REQUIRE(session.player().on_ground);

        InputState right;
        right.right = true;
        runUntilLeaving(session, right, "room_01", 1200);

        CHECK(session.currentRoom().id() == "room_02");
        run(session, InputState{}, 120);  // it arrives in the air, high up: let it fall
        CHECK(session.player().on_ground);
    }
}

namespace {

// One room with a floor and a coin. The spikes stand on the floor: either one
// small patch in the middle, or all along it (`everywhere`).
RoomGraph makeSpikeWorld(bool everywhere = false) {
    using domain::AABB;
    const AABB spikes = everywhere ? AABB{0.0F, 211.0F, 320.0F, 5.0F} : AABB{100.0F, 211.0F, 8.0F, 5.0F};
    std::vector<Room> rooms;
    rooms.emplace_back("only", test_maps::makeMap(), std::map<Side, domain::RoomId>{},
                       std::vector<domain::Coin>{domain::Coin{40.0F, 200.0F}},
                       std::vector<domain::Hazard>{domain::Hazard{spikes}});
    return RoomGraph(std::move(rooms));
}

int hurtCount(const std::vector<application::GameEvent>& events) {
    int count = 0;
    for (const auto& event : events) {
        count += std::holds_alternative<application::PlayerHurt>(event) ? 1 : 0;
    }
    return count;
}

int restartCount(const std::vector<application::GameEvent>& events) {
    int count = 0;
    for (const auto& event : events) {
        count += std::holds_alternative<application::GameRestarted>(event) ? 1 : 0;
    }
    return count;
}

}  // namespace

TEST_CASE("GameSession hazards and health") {
    SUBCASE("touching spikes costs one heart and reports it") {
        // Standing on the floor, right on the spikes.
        GameSession session(makeSpikeWorld(), "only", 100.0F, test_maps::kFloorTop - 16.0F);
        REQUIRE(session.player().health == domain::kMaxHealth);

        const auto events = session.update(InputState{});

        REQUIRE(hurtCount(events) == 1);
        const auto* hurt = std::get_if<application::PlayerHurt>(&events[0]);
        REQUIRE(hurt != nullptr);
        CHECK(hurt->room == "only");
        CHECK(hurt->health_left == domain::kMaxHealth - 1);
        CHECK(session.player().health == domain::kMaxHealth - 1);
    }

    SUBCASE("the player is thrown back, away from the spikes") {
        // Just left of the spikes' centre: it must be thrown to the left.
        GameSession session(makeSpikeWorld(), "only", 98.0F, test_maps::kFloorTop - 16.0F);
        const float before = session.player().x;
        run(session, InputState{}, domain::kHurtSteps + 2);
        CHECK(session.player().x < before - 5.0F);   // thrown to the left (not toward the spikes)...
        CHECK(session.player().x > before - 12.0F);  // ...but only a little
    }

    SUBCASE("one hit per invulnerability window, even if it keeps touching the spikes") {
        // Spikes all along the floor: the player stays on them the whole time.
        GameSession session(makeSpikeWorld(true), "only", 100.0F, test_maps::kFloorTop - 16.0F);
        int hits = 0;
        for (int step = 0; step < domain::kInvulnerableSteps - 5; ++step) {
            hits += hurtCount(session.update(InputState{}));
        }
        CHECK(hits == 1);
    }

    SUBCASE("losing the last heart restarts the whole game") {
        // Spawns in the air above the coin; spikes all along the floor. It lands
        // (collecting the coin, and taking the first hit) and keeps getting hurt
        // each time the invulnerability wears off.
        GameSession session(makeSpikeWorld(true), "only", 40.0F, 60.0F);

        int hits = 0;
        int restarts = 0;
        bool had_coin = false;
        for (int step = 0; step < 60 * 30 && restarts == 0; ++step) {
            const auto events = session.update(InputState{});
            had_coin = had_coin || session.collectedCoins().count() == 1;
            hits += hurtCount(events);
            restarts += restartCount(events);
        }

        CHECK(had_coin);
        CHECK(restarts == 1);
        CHECK(hits == domain::kMaxHealth);  // three hits, the last one ends it
        CHECK(session.player().health == domain::kMaxHealth);
        CHECK(session.currentRoom().id() == "only");
        CHECK(session.collectedCoins().count() == 0);  // the coin is back
        CHECK(session.player().x == doctest::Approx(40.0F));
        CHECK(session.player().y == doctest::Approx(60.0F));
        CHECK(session.player().invulnerable_steps == 0);
    }
}

namespace {

// A ceiling tile at (12, 5) with a stalactite trap hanging right under it.
RoomGraph makeTrapWorld() {
    std::vector<Room> rooms;
    rooms.emplace_back("only", test_maps::makeMap({{12, 5}}), std::map<Side, domain::RoomId>{},
                       std::vector<domain::Coin>{}, std::vector<domain::Hazard>{},
                       std::vector<domain::FallingStalactite>{
                           domain::FallingStalactite{domain::AABB{96.0F, 48.0F, 8.0F, 8.0F}}});
    return RoomGraph(std::move(rooms));
}

int fellCount(const std::vector<application::GameEvent>& events) {
    int count = 0;
    for (const auto& event : events) {
        count += std::holds_alternative<application::StalactiteFell>(event) ? 1 : 0;
    }
    return count;
}

}  // namespace

TEST_CASE("GameSession stalactite traps") {
    // Standing right under it, on the floor.
    GameSession session(makeTrapWorld(), "only", 96.0F, test_maps::kFloorTop - 16.0F);
    const auto phase = [&session]() { return session.stalactiteTraps().state("only", 0).phase; };

    SUBCASE("it warns first, then falls and reports it once") {
        session.update(InputState{});
        CHECK(phase() == domain::StalactitePhase::Shaking);

        int fell = 0;
        int steps_until_fall = 0;
        for (int step = 0; step < 100 && fell == 0; ++step) {
            fell += fellCount(session.update(InputState{}));
            ++steps_until_fall;
        }
        CHECK(fell == 1);
        CHECK(steps_until_fall >= domain::kShakeSteps - 2);  // the player had time to react
        CHECK(phase() == domain::StalactitePhase::Falling);
    }

    SUBCASE("a player who stays under it is hurt once, and the stalactite shatters on them") {
        int hits = 0;
        for (int step = 0; step < 300; ++step) {
            hits += hurtCount(session.update(InputState{}));
        }
        CHECK(hits == 1);
        CHECK(session.player().health == domain::kMaxHealth - 1);
        CHECK(phase() == domain::StalactitePhase::Gone);
    }

    SUBCASE("a player who walks away during the warning is not hurt") {
        InputState right;
        right.right = true;
        int hits = 0;
        for (int step = 0; step < 300; ++step) {
            hits += hurtCount(session.update(right));
        }
        CHECK(hits == 0);
        CHECK(session.player().health == domain::kMaxHealth);
        CHECK(phase() == domain::StalactitePhase::Gone);  // it fell anyway, and missed
    }

    SUBCASE("restarting the game puts the trap back") {
        // Same trap, plus spikes all along the floor: the player is hurt by the
        // spikes until it loses everything, long after the stalactite has fallen.
        std::vector<Room> rooms;
        rooms.emplace_back("only", test_maps::makeMap({{12, 5}}), std::map<Side, domain::RoomId>{},
                           std::vector<domain::Coin>{},
                           std::vector<domain::Hazard>{domain::Hazard{domain::AABB{0.0F, 211.0F, 320.0F, 5.0F}}},
                           std::vector<domain::FallingStalactite>{
                               domain::FallingStalactite{domain::AABB{96.0F, 48.0F, 8.0F, 8.0F}}});
        GameSession doomed(RoomGraph(std::move(rooms)), "only", 96.0F, 60.0F);

        bool shattered_first = false;
        int restarts = 0;
        for (int step = 0; step < 60 * 30 && restarts == 0; ++step) {
            restarts += restartCount(doomed.update(InputState{}));
            shattered_first = shattered_first ||
                              doomed.stalactiteTraps().state("only", 0).phase == domain::StalactitePhase::Gone;
        }
        CHECK(shattered_first);  // it had fallen by then
        REQUIRE(restarts == 1);
        CHECK(doomed.stalactiteTraps().state("only", 0).phase == domain::StalactitePhase::Idle);  // and is back
    }
}

namespace {

// One room: a floor, spikes under the spawn point (x = 100) and a fruit further right.
RoomGraph makeFruitWorld(bool spikes_everywhere = false) {
    using domain::AABB;
    const AABB spikes = spikes_everywhere ? AABB{0.0F, 211.0F, 320.0F, 5.0F} : AABB{100.0F, 211.0F, 8.0F, 5.0F};
    std::vector<Room> rooms;
    rooms.emplace_back("only", test_maps::makeMap(), std::map<Side, domain::RoomId>{},
                       std::vector<domain::Coin>{}, std::vector<domain::Hazard>{domain::Hazard{spikes}},
                       std::vector<domain::FallingStalactite>{},
                       std::vector<domain::Fruit>{domain::Fruit{180.0F, 208.0F}});
    return RoomGraph(std::move(rooms));
}

std::vector<application::FruitEaten> eatenIn(const std::vector<application::GameEvent>& events) {
    std::vector<application::FruitEaten> eaten;
    for (const auto& event : events) {
        if (const auto* fruit = std::get_if<application::FruitEaten>(&event)) {
            eaten.push_back(*fruit);
        }
    }
    return eaten;
}

}  // namespace

TEST_CASE("GameSession healing fruits") {
    SUBCASE("a fruit heals one heart after a hit and reports it") {
        // Spawns on the spikes: it is hurt at once (2 hearts), then walks to the fruit.
        GameSession session(makeFruitWorld(), "only", 100.0F, test_maps::kFloorTop - 16.0F);
        session.update(InputState{});
        REQUIRE(session.player().health == domain::kMaxHealth - 1);

        InputState right;
        right.right = true;
        std::vector<application::FruitEaten> eaten;
        for (int step = 0; step < 200 && eaten.empty(); ++step) {
            const auto found = eatenIn(session.update(right));
            eaten.insert(eaten.end(), found.begin(), found.end());
        }

        REQUIRE(eaten.size() == 1);
        CHECK(eaten[0].room == "only");
        CHECK(eaten[0].fruit_index == 0);
        CHECK(eaten[0].health_after == domain::kMaxHealth);
        CHECK(eaten[0].hearts_gained == 1);
        CHECK(session.player().health == domain::kMaxHealth);
        CHECK(session.eatenFruits().count() == 1);
    }

    SUBCASE("at full health the fruit is eaten anyway and wasted") {
        GameSession session(makeFruitWorld(), "only", 180.0F, test_maps::kFloorTop - 16.0F);
        REQUIRE(session.player().health == domain::kMaxHealth);

        const auto eaten = eatenIn(session.update(InputState{}));

        REQUIRE(eaten.size() == 1);
        CHECK(eaten[0].hearts_gained == 0);
        CHECK(eaten[0].health_after == domain::kMaxHealth);
        CHECK(session.player().health == domain::kMaxHealth);
        CHECK(session.eatenFruits().count() == 1);  // it is gone
    }

    SUBCASE("a fruit is eaten only once") {
        GameSession session(makeFruitWorld(), "only", 180.0F, test_maps::kFloorTop - 16.0F);
        int total = 0;
        for (int step = 0; step < 60; ++step) {
            total += static_cast<int>(eatenIn(session.update(InputState{})).size());
        }
        CHECK(total == 1);
    }

    SUBCASE("the game restarting puts the fruit back") {
        // Spikes along the whole floor: it eats the fruit (wasted), then loses every heart.
        GameSession session(makeFruitWorld(true), "only", 180.0F, 60.0F);
        bool ate = false;
        int restarts = 0;
        for (int step = 0; step < 60 * 30 && restarts == 0; ++step) {
            const auto events = session.update(InputState{});
            ate = ate || !eatenIn(events).empty();
            restarts += restartCount(events);
        }
        CHECK(ate);
        REQUIRE(restarts == 1);
        CHECK(session.eatenFruits().count() == 0);
        CHECK(session.player().health == domain::kMaxHealth);
    }
}

namespace {

template <class Event>
int countOfType(const std::vector<application::GameEvent>& events) {
    int count = 0;
    for (const auto& event : events) {
        count += std::holds_alternative<Event>(event) ? 1 : 0;
    }
    return count;
}

// Runs `steps` steps and gathers every event, in order.
std::vector<application::GameEvent> collect(GameSession& session, const InputState& input, int steps) {
    std::vector<application::GameEvent> all;
    for (int i = 0; i < steps; ++i) {
        const auto events = session.update(input);
        all.insert(all.end(), events.begin(), events.end());
    }
    return all;
}

}  // namespace

TEST_CASE("GameSession announces jumps and landings") {
    GameSession session(makePlusWorld(), "center", 100.0F, test_maps::kFloorTop - 16.0F);  // on the floor
    CHECK(countOfType<application::PlayerLanded>(collect(session, InputState{}, 30)) == 0);  // standing: nothing

    SUBCASE("a jump is announced once, and not again while the button stays held") {
        InputState jump;
        jump.jump = true;
        const auto events = collect(session, jump, 40);  // holds the button through the whole jump
        CHECK(countOfType<application::PlayerJumped>(events) == 1);
    }

    SUBCASE("coming back down is a landing, with the speed of the fall") {
        InputState jump;
        jump.jump = true;
        auto events = collect(session, jump, 1);
        const auto later = collect(session, InputState{}, 120);
        events.insert(events.end(), later.begin(), later.end());

        REQUIRE(countOfType<application::PlayerLanded>(events) == 1);
        for (const auto& event : events) {
            if (const auto* landed = std::get_if<application::PlayerLanded>(&event)) {
                CHECK(landed->impact_speed >= 1.5F);
            }
        }
    }

    SUBCASE("walking on flat ground announces neither jumps nor landings") {
        InputState right;
        right.right = true;
        const auto events = collect(session, right, 60);
        CHECK(countOfType<application::PlayerJumped>(events) == 0);
        CHECK(countOfType<application::PlayerLanded>(events) == 0);
    }

    SUBCASE("the first fall from the spawn point in mid-air is a landing") {
        GameSession falling(makePlusWorld(), "center", 100.0F, 60.0F);
        CHECK(countOfType<application::PlayerLanded>(collect(falling, InputState{}, 120)) == 1);
    }
}

TEST_CASE("GameSession announces the start of a slide") {
    std::vector<Room> rooms;
    for (const char* name : {"room_01", "room_02", "room_03", "room_04"}) {
        rooms.push_back(infrastructure::loadTiledRoom(std::string(PLATFORMER_ASSETS_DIR) + "/tiled/" + name + ".tmj"));
    }
    GameSession session(RoomGraph(std::move(rooms)), "room_01", 244.0F, 100.0F);  // above the top of the ramp (clear of the platform over it)
    collect(session, InputState{}, 120);                                          // it lands on the ramp
    REQUIRE(session.player().on_ground);

    SUBCASE("down on the ramp starts a slide, announced once") {
        InputState down;
        down.down = true;
        const auto events = collect(session, down, 10);
        CHECK(countOfType<application::SlideStarted>(events) == 1);
    }

    SUBCASE("walking on the ramp does not") {
        InputState left;
        left.left = true;
        CHECK(countOfType<application::SlideStarted>(collect(session, left, 10)) == 0);
    }
}

TEST_CASE("GameSession announces the stages of a stalactite trap") {
    GameSession session(makeTrapWorld(), "only", 96.0F, test_maps::kFloorTop - 16.0F);  // right under it

    SUBCASE("warning, then it comes loose, then it shatters, in that order, once each") {
        const auto events = collect(session, InputState{}, 300);
        REQUIRE(countOfType<application::StalactiteWarned>(events) == 1);
        REQUIRE(countOfType<application::StalactiteFell>(events) == 1);
        REQUIRE(countOfType<application::StalactiteShattered>(events) == 1);

        int warned_at = -1;
        int fell_at = -1;
        int shattered_at = -1;
        for (std::size_t i = 0; i < events.size(); ++i) {
            if (std::holds_alternative<application::StalactiteWarned>(events[i])) warned_at = static_cast<int>(i);
            if (std::holds_alternative<application::StalactiteFell>(events[i])) fell_at = static_cast<int>(i);
            if (std::holds_alternative<application::StalactiteShattered>(events[i])) shattered_at = static_cast<int>(i);
        }
        CHECK(warned_at < fell_at);
        CHECK(fell_at < shattered_at);
    }

    SUBCASE("it shatters on the floor too when the player has walked away") {
        InputState right;
        right.right = true;
        const auto events = collect(session, right, 300);
        CHECK(countOfType<application::StalactiteWarned>(events) == 1);
        CHECK(countOfType<application::StalactiteShattered>(events) == 1);
        CHECK(countOfType<application::PlayerHurt>(events) == 0);
    }

    SUBCASE("nothing is announced while nobody is under it") {
        GameSession far_away(makeTrapWorld(), "only", 10.0F, test_maps::kFloorTop - 16.0F);
        const auto events = collect(far_away, InputState{}, 200);
        CHECK(countOfType<application::StalactiteWarned>(events) == 0);
    }
}

namespace {

// One room with a floor and a walker standing on it at x = 140 (it starts going left).
RoomGraph makeEnemyWorld(float enemy_x = 140.0F) {
    using domain::AABB;
    std::vector<Room> rooms;
    rooms.emplace_back("only", test_maps::makeMap(), std::map<Side, domain::RoomId>{},
                       std::vector<domain::Coin>{}, std::vector<domain::Hazard>{},
                       std::vector<domain::FallingStalactite>{}, std::vector<domain::Fruit>{},
                       std::vector<domain::Enemy>{domain::Enemy{AABB{enemy_x, test_maps::kFloorTop - 8.0F, 8.0F, 8.0F}}});
    return RoomGraph(std::move(rooms));
}

template <class Event>
int countOf(const std::vector<application::GameEvent>& events) {
    int count = 0;
    for (const auto& event : events) {
        count += std::holds_alternative<Event>(event) ? 1 : 0;
    }
    return count;
}

}  // namespace

TEST_CASE("GameSession enemies") {
    SUBCASE("an enemy walks while the session runs") {
        GameSession session(makeEnemyWorld(), "only", 20.0F, test_maps::kFloorTop - 16.0F);
        run(session, InputState{}, 30);
        CHECK(session.enemyPatrols().state("only", 0).x_offset < 0.0F);  // it set off to the left
        CHECK(session.player().health == domain::kMaxHealth);
    }

    SUBCASE("walking into an enemy hurts the player and throws them back") {
        GameSession session(makeEnemyWorld(), "only", 120.0F, test_maps::kFloorTop - 16.0F);
        InputState right;
        right.right = true;
        std::vector<application::GameEvent> all;
        for (int step = 0; step < 120 && countOf<application::PlayerHurt>(all) == 0; ++step) {
            const auto events = session.update(right);
            all.insert(all.end(), events.begin(), events.end());
        }
        CHECK(countOf<application::PlayerHurt>(all) == 1);
        CHECK(session.player().health == domain::kMaxHealth - 1);
        CHECK(session.player().invulnerable_steps > 0);
        CHECK(session.enemyPatrols().state("only", 0).alive);  // a side hit does not hurt the enemy
        CHECK(session.player().hurt_direction == -1);          // thrown away from it, to the left
    }

    SUBCASE("landing on an enemy defeats it and bounces the player") {
        // The player falls straight onto the enemy's head.
        GameSession session(makeEnemyWorld(), "only", 140.0F, test_maps::kFloorTop - 8.0F - 16.0F - 20.0F);
        std::vector<application::GameEvent> all;
        for (int step = 0; step < 60 && countOf<application::EnemyStomped>(all) == 0; ++step) {
            const auto events = session.update(InputState{});
            all.insert(all.end(), events.begin(), events.end());
        }
        REQUIRE(countOf<application::EnemyStomped>(all) == 1);
        CHECK_FALSE(session.enemyPatrols().state("only", 0).alive);
        CHECK(session.player().health == domain::kMaxHealth);  // unhurt
        CHECK(countOf<application::PlayerHurt>(all) == 0);
        CHECK(session.player().velocity_y < 0.0F);             // going up
        CHECK_FALSE(session.player().on_ground);
    }

    SUBCASE("a defeated enemy no longer hurts, and is reported once") {
        GameSession session(makeEnemyWorld(), "only", 140.0F, test_maps::kFloorTop - 8.0F - 16.0F - 20.0F);
        std::vector<application::GameEvent> all;
        for (int step = 0; step < 200; ++step) {
            const auto events = session.update(InputState{});
            all.insert(all.end(), events.begin(), events.end());
        }
        CHECK(countOf<application::EnemyStomped>(all) == 1);
        CHECK(countOf<application::PlayerHurt>(all) == 0);
    }

    SUBCASE("the total restart brings defeated enemies back") {
        // Spikes cover the floor right of the enemy. The player stomps it, then walks into
        // the spikes until every heart is gone.
        using domain::AABB;
        std::vector<Room> rooms;
        rooms.emplace_back("only", test_maps::makeMap(), std::map<Side, domain::RoomId>{},
                           std::vector<domain::Coin>{},
                           std::vector<domain::Hazard>{domain::Hazard{AABB{160.0F, 211.0F, 160.0F, 5.0F}}},
                           std::vector<domain::FallingStalactite>{}, std::vector<domain::Fruit>{},
                           std::vector<domain::Enemy>{
                               domain::Enemy{AABB{140.0F, test_maps::kFloorTop - 8.0F, 8.0F, 8.0F}}});
        GameSession session{RoomGraph(std::move(rooms)), "only", 140.0F,
                            test_maps::kFloorTop - 8.0F - 16.0F - 20.0F};
        InputState right;
        right.right = true;
        bool stomped = false;
        bool restarted = false;
        for (int step = 0; step < 2000 && !restarted; ++step) {
            const auto events = session.update(stomped ? right : InputState{});
            stomped = stomped || countOf<application::EnemyStomped>(events) > 0;
            restarted = countOf<application::GameRestarted>(events) > 0;
        }
        REQUIRE(stomped);
        REQUIRE(restarted);
        CHECK(session.enemyPatrols().state("only", 0).alive);
        CHECK(session.player().health == domain::kMaxHealth);
    }
}
