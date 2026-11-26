#include <doctest/doctest.h>

#include <variant>

#include "application/game_session.h"
#include "infrastructure/assets/tiled_map_loader.h"
#include "level_tools/text_level.h"
#include "presentation/animation.h"
#include "test_maps.h"

using application::GameSession;
using domain::InputState;
using domain::Orb;
using domain::OrbColour;
using domain::Room;
using domain::RoomGraph;

namespace {

constexpr float kFeet = test_maps::kFloorTop - 16.0F;

// One room, all on the floor from left to right: orb (x = 60), exit (x = 120), orb (x = 180).
RoomGraph makeWorld() {
    std::vector<Room> rooms;
    rooms.emplace_back("only", test_maps::makeMap(), std::map<domain::Side, domain::RoomId>{},
                       std::vector<domain::Coin>{}, std::vector<domain::Hazard>{},
                       std::vector<domain::FallingStalactite>{}, std::vector<domain::Fruit>{},
                       std::vector<domain::Enemy>{}, std::vector<domain::Checkpoint>{},
                       std::vector<domain::Exit>{domain::Exit{120.0F, 208.0F}},
                       std::vector<Orb>{Orb{60.0F, 200.0F, 16.0F, OrbColour::Red},
                                        Orb{180.0F, 200.0F, 16.0F, OrbColour::Green}});
    return RoomGraph(std::move(rooms));
}

// Walks in one direction for a while and returns everything that happened.
std::vector<application::GameEvent> walk(GameSession& session, int direction, int steps) {
    InputState input;
    input.left = direction < 0;
    input.right = direction > 0;
    std::vector<application::GameEvent> all;
    for (int step = 0; step < steps && !session.won(); ++step) {
        for (const auto& event : session.update(input)) {
            all.push_back(event);
        }
    }
    return all;
}

}  // namespace

TEST_CASE("taking an orb reports its colour and how many are left, once") {
    GameSession session(makeWorld(), "only", 40.0F, kFeet);
    CHECK(session.totalOrbs() == 2);
    CHECK_FALSE(session.hasTakenOrb(OrbColour::Red));

    int events = 0;
    application::OrbCollected taken{};
    for (const auto& event : walk(session, 1, 30)) {
        if (const auto* orb = std::get_if<application::OrbCollected>(&event)) {
            ++events;
            taken = *orb;
        }
    }
    CHECK(events == 1);
    CHECK(taken.colour == OrbColour::Red);
    CHECK(taken.collected == 1);
    CHECK(taken.total == 2);
    CHECK(session.hasTakenOrb(OrbColour::Red));
    CHECK_FALSE(session.hasTakenOrb(OrbColour::Green));
}

TEST_CASE("the exit stays shut until every orb is taken") {
    GameSession session(makeWorld(), "only", 40.0F, kFeet);
    CHECK_FALSE(session.exitOpen());

    walk(session, 1, 60);  // takes the red orb and goes over the exit
    CHECK(session.player().x > 120.0F);
    CHECK_FALSE(session.won());

    walk(session, 1, 80);  // on to the green orb
    CHECK(session.collectedOrbs().count() == 2);
    CHECK(session.exitOpen());
    CHECK_FALSE(session.won());

    walk(session, -1, 120);  // back to the exit
    CHECK(session.won());
}

TEST_CASE("a world without orbs has its exit open from the start") {
    std::vector<Room> rooms;
    rooms.emplace_back("only", test_maps::makeMap());
    const GameSession session(RoomGraph(std::move(rooms)), "only", 40.0F, kFeet);
    CHECK(session.exitOpen());
}

TEST_CASE("orbs are read from Tiled with their colour") {
    const std::string map = R"({
      "width": 4, "height": 4, "tilewidth": 8, "tileheight": 8,
      "layers": [
        { "type": "tilelayer", "data": [0,0,0,0, 0,0,0,0, 0,0,0,0, 1,1,1,1] },
        { "type": "objectgroup", "objects": [
          { "id": 1, "class": "orb", "x": 4, "y": 8, "width": 16, "height": 16,
            "properties": [ { "name": "colour", "type": "string", "value": "yellow" } ] } ] }
      ] })";
    const Room room = infrastructure::parseTiledRoom("r", map);
    REQUIRE(room.orbs().size() == 1);
    CHECK(room.orbs()[0].colour == OrbColour::Yellow);
    CHECK(room.orbs()[0].x == 4.0F);
    CHECK(room.orbs()[0].size == 16.0F);

    std::string unknown = map;
    unknown.replace(unknown.find("yellow"), 6, "purple");
    CHECK_THROWS_WITH_AS(infrastructure::parseTiledRoom("r", unknown), doctest::Contains("colour"),
                         std::runtime_error);
}

TEST_CASE("orbs (R G Y) are two tiles big and survive text -> tmj -> game -> text") {
    const auto level = level_tools::parseTextLevel("---\n........\n.R..G.Y.\n########\n");
    REQUIRE(level.orbs.size() == 3);
    CHECK(level.orbs[0].x == 4.0F);
    CHECK(level.orbs[0].y == 0.0F);
    CHECK(level.orbs[0].size == 16.0F);

    const Room room = infrastructure::parseTiledRoom("r", level_tools::toTiledJson(level));
    REQUIRE(room.orbs().size() == 3);
    const auto again = level_tools::parseTextLevel(level_tools::toText(room));
    REQUIRE(again.orbs.size() == 3);
    for (std::size_t i = 0; i < 3; ++i) {
        CHECK(again.orbs[i].colour == level.orbs[i].colour);
        CHECK(again.orbs[i].x == level.orbs[i].x);
        CHECK(again.orbs[i].y == level.orbs[i].y);
    }
}

TEST_CASE("orb animation") {
    for (std::uint64_t time = 0; time < 5000; time += 37) {
        for (const auto colour : {OrbColour::Red, OrbColour::Green, OrbColour::Yellow}) {
            CHECK(presentation::orbFrame(colour, time) >= 0);
            CHECK(presentation::orbFrame(colour, time) < presentation::kOrbFrames);
            CHECK(std::abs(presentation::orbBob(colour, time)) <= 2);
        }
        CHECK(presentation::orbFrame(OrbColour::Red, time) != presentation::orbFrame(OrbColour::Green, time));
    }
}

TEST_CASE("the exit's portcullis rises once the open exit comes on screen") {
    using namespace presentation;
    CHECK(exitSpriteFrame(false, 100000, 0) == kExitClosedFrame);
    CHECK(exitSpriteFrame(true, 0, 0) == kExitClosedFrame);  // a moment to see it shut
    CHECK(exitSpriteFrame(true, kGateDelayMs - 1, 0) == kExitClosedFrame);
    CHECK(exitSpriteFrame(true, kGateDelayMs, 0) == kGateFirstFrame);

    int previous = kGateFirstFrame;
    for (std::uint64_t ms = kGateDelayMs; ms < kGateDelayMs + kGateFrames * kGateFrameMs; ms += 17) {
        const int frame = exitSpriteFrame(true, ms, ms);
        CHECK(frame >= previous);  // it only goes up
        CHECK(frame < kGateFirstFrame + kGateFrames);
        previous = frame;
    }
    CHECK(previous == kGateFirstFrame + kGateFrames - 1);

    const int open = exitSpriteFrame(true, kGateDelayMs + kGateFrames * kGateFrameMs, 600);
    CHECK(open == exitFrame(600));  // and then the daylight pulses
}

TEST_CASE("the open exit is announced once each time its room is entered") {
    const auto announced = [](const std::vector<application::GameEvent>& events) {
        int count = 0;
        for (const auto& event : events) {
            count += std::holds_alternative<application::GateOpened>(event) ? 1 : 0;
        }
        return count;
    };

    GameSession shut(makeWorld(), "only", 40.0F, kFeet);
    CHECK(announced(walk(shut, 1, 30)) == 0);  // orbs missing

    std::vector<Room> rooms;
    rooms.emplace_back("only", test_maps::makeMap(), std::map<domain::Side, domain::RoomId>{},
                       std::vector<domain::Coin>{}, std::vector<domain::Hazard>{},
                       std::vector<domain::FallingStalactite>{}, std::vector<domain::Fruit>{},
                       std::vector<domain::Enemy>{}, std::vector<domain::Checkpoint>{},
                       std::vector<domain::Exit>{domain::Exit{300.0F, 208.0F}});
    GameSession open(RoomGraph(std::move(rooms)), "only", 40.0F, kFeet);
    CHECK(announced(walk(open, 1, 30)) == 1);
    CHECK(announced(walk(open, -1, 30)) == 0);  // still the same visit
}
