#include "infrastructure/assets/tiled_map_loader.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace infrastructure {

namespace {

using nlohmann::json;
using domain::Side;

// Tiled stores flip/rotation flags in the top bits of each tile id.
// We only care about the id itself for now.
constexpr std::uint32_t kTileIdMask = 0x1FFFFFFFU;

const json& findFirstTileLayer(const json& map) {
    for (const json& layer : map.at("layers")) {
        if (layer.at("type").get<std::string>() == "tilelayer") {
            return layer;
        }
    }
    throw std::runtime_error("Tiled map has no tile layer");
}

domain::TileMap readTiles(const json& map) {
    const int width = map.at("width").get<int>();
    const int height = map.at("height").get<int>();
    const int tile_width = map.at("tilewidth").get<int>();
    const int tile_height = map.at("tileheight").get<int>();

    if (tile_width != tile_height) {
        throw std::runtime_error("Tiled map must use square tiles");
    }
    if (map.value("infinite", false)) {
        throw std::runtime_error("Infinite Tiled maps are not supported");
    }

    const json& layer = findFirstTileLayer(map);
    if (!layer.at("data").is_array()) {
        throw std::runtime_error("Tile layer data must be uncompressed (use CSV encoding)");
    }

    std::vector<int> tiles;
    tiles.reserve(layer.at("data").size());
    for (const json& raw : layer.at("data")) {
        tiles.push_back(static_cast<int>(raw.get<std::uint32_t>() & kTileIdMask));
    }
    return domain::TileMap(width, height, tile_width, std::move(tiles));
}

std::map<Side, domain::RoomId> readNeighbors(const json& map) {
    static const std::map<std::string, Side> kSideNames = {
        {"north", Side::North}, {"south", Side::South}, {"east", Side::East}, {"west", Side::West}};

    std::map<Side, domain::RoomId> neighbors;
    if (!map.contains("properties")) {
        return neighbors;
    }
    for (const json& property : map.at("properties")) {
        const auto side = kSideNames.find(property.at("name").get<std::string>());
        if (side == kSideNames.end()) {
            continue;  // unrelated custom property
        }
        const auto target = property.at("value").get<std::string>();
        if (!target.empty()) {
            neighbors.emplace(side->second, target);
        }
    }
    return neighbors;
}

// Coins are rectangle objects of class "coin" in any object layer.
// (Tiled >= 1.9 calls the field "class"; older versions call it "type".)
std::vector<domain::Coin> readCoins(const json& map) {
    std::vector<domain::Coin> coins;
    for (const json& layer : map.at("layers")) {
        if (layer.at("type").get<std::string>() != "objectgroup") {
            continue;
        }
        for (const json& object : layer.at("objects")) {
            const std::string kind = object.value("class", object.value("type", std::string()));
            if (kind != "coin") {
                continue;
            }
            domain::Coin coin;
            coin.x = object.at("x").get<float>();
            coin.y = object.at("y").get<float>();
            const float width = object.value("width", 0.0F);
            if (width > 0.0F) {
                coin.size = width;
            }
            coins.push_back(coin);
        }
    }
    return coins;
}

// Healing fruits are rectangle objects of class "fruit", read like coins.
std::vector<domain::Fruit> readFruits(const json& map) {
    std::vector<domain::Fruit> fruits;
    for (const json& layer : map.at("layers")) {
        if (layer.at("type").get<std::string>() != "objectgroup") {
            continue;
        }
        for (const json& object : layer.at("objects")) {
            const std::string kind = object.value("class", object.value("type", std::string()));
            if (kind != "fruit") {
                continue;
            }
            domain::Fruit fruit;
            fruit.x = object.at("x").get<float>();
            fruit.y = object.at("y").get<float>();
            const float width = object.value("width", 0.0F);
            if (width > 0.0F) {
                fruit.size = width;
            }
            fruits.push_back(fruit);
        }
    }
    return fruits;
}

// Enemies are rectangle objects of class "walker" or "bat": the rectangle is where it starts and
// the size of its hitbox. A bat may have a number property "detect_radius": how near (px) the
// player must come for it to attack.
std::vector<domain::Enemy> readEnemies(const json& map) {
    std::vector<domain::Enemy> enemies;
    for (const json& layer : map.at("layers")) {
        if (layer.at("type").get<std::string>() != "objectgroup") {
            continue;
        }
        for (const json& object : layer.at("objects")) {
            const std::string kind = object.value("class", object.value("type", std::string()));
            if (kind != "walker" && kind != "bat") {
                continue;
            }
            const float width = object.value("width", 0.0F);
            const float height = object.value("height", 0.0F);
            if (width <= 0.0F || height <= 0.0F) {
                throw std::invalid_argument("a '" + kind + "' object must be a rectangle with a size");
            }
            domain::Enemy enemy{domain::AABB{object.at("x").get<float>(), object.at("y").get<float>(), width, height}};
            if (kind == "bat") {
                enemy.kind = domain::EnemyKind::Bat;
                if (object.contains("properties")) {
                    for (const json& property : object.at("properties")) {
                        if (property.at("name").get<std::string>() == "detect_radius") {
                            enemy.detect_radius = property.at("value").get<float>();
                        }
                    }
                }
                if (enemy.detect_radius < 0.0F) {
                    throw std::invalid_argument("a bat's 'detect_radius' cannot be negative");
                }
            }
            enemies.push_back(enemy);
        }
    }
    return enemies;
}

// Checkpoints are rectangle objects of class "checkpoint", read like coins.
std::vector<domain::Checkpoint> readCheckpoints(const json& map) {
    std::vector<domain::Checkpoint> checkpoints;
    for (const json& layer : map.at("layers")) {
        if (layer.at("type").get<std::string>() != "objectgroup") {
            continue;
        }
        for (const json& object : layer.at("objects")) {
            const std::string kind = object.value("class", object.value("type", std::string()));
            if (kind != "checkpoint") {
                continue;
            }
            domain::Checkpoint checkpoint;
            checkpoint.x = object.at("x").get<float>();
            checkpoint.y = object.at("y").get<float>();
            const float width = object.value("width", 0.0F);
            if (width > 0.0F) {
                checkpoint.size = width;
            }
            checkpoints.push_back(checkpoint);
        }
    }
    return checkpoints;
}

// Exits are rectangle objects of class "exit", read like coins.
std::vector<domain::Exit> readExits(const json& map) {
    std::vector<domain::Exit> exits;
    for (const json& layer : map.at("layers")) {
        if (layer.at("type").get<std::string>() != "objectgroup") {
            continue;
        }
        for (const json& object : layer.at("objects")) {
            const std::string kind = object.value("class", object.value("type", std::string()));
            if (kind != "exit") {
                continue;
            }
            domain::Exit exit;
            exit.x = object.at("x").get<float>();
            exit.y = object.at("y").get<float>();
            const float width = object.value("width", 0.0F);
            if (width > 0.0F) {
                exit.size = width;
            }
            exits.push_back(exit);
        }
    }
    return exits;
}

// Orbs are rectangle objects of class "orb" with a string property "colour": red, green or yellow.
std::vector<domain::Orb> readOrbs(const json& map) {
    static const std::map<std::string, domain::OrbColour> kColours = {
        {"red", domain::OrbColour::Red}, {"green", domain::OrbColour::Green}, {"yellow", domain::OrbColour::Yellow}};
    std::vector<domain::Orb> orbs;
    for (const json& layer : map.at("layers")) {
        if (layer.at("type").get<std::string>() != "objectgroup") {
            continue;
        }
        for (const json& object : layer.at("objects")) {
            const std::string kind = object.value("class", object.value("type", std::string()));
            if (kind != "orb") {
                continue;
            }
            std::string colour;
            if (object.contains("properties")) {
                for (const json& property : object.at("properties")) {
                    if (property.at("name").get<std::string>() == "colour") {
                        colour = property.at("value").get<std::string>();
                    }
                }
            }
            const auto found = kColours.find(colour);
            if (found == kColours.end()) {
                throw std::invalid_argument("an 'orb' object needs a 'colour' property: red, green or yellow");
            }
            domain::Orb orb;
            orb.x = object.at("x").get<float>();
            orb.y = object.at("y").get<float>();
            const float width = object.value("width", 0.0F);
            if (width > 0.0F) {
                orb.size = width;
            }
            orb.colour = found->second;
            orbs.push_back(orb);
        }
    }
    return orbs;
}

// Spikes are rectangle objects of class "spikes": the rectangle IS the hitbox.
std::vector<domain::Hazard> readHazards(const json& map) {
    std::vector<domain::Hazard> hazards;
    for (const json& layer : map.at("layers")) {
        if (layer.at("type").get<std::string>() != "objectgroup") {
            continue;
        }
        for (const json& object : layer.at("objects")) {
            const std::string kind = object.value("class", object.value("type", std::string()));
            if (kind != "spikes") {
                continue;
            }
            const float width = object.value("width", 0.0F);
            const float height = object.value("height", 0.0F);
            if (width <= 0.0F || height <= 0.0F) {
                throw std::invalid_argument("a 'spikes' object must be a rectangle with a size");
            }
            hazards.push_back(domain::Hazard{domain::AABB{object.at("x").get<float>(),
                                                          object.at("y").get<float>(), width, height}});
        }
    }
    return hazards;
}

// Stalactite traps are rectangle objects of class "falling_stalactite": the
// rectangle is where it hangs and its hitbox.
std::vector<domain::FallingStalactite> readStalactites(const json& map) {
    std::vector<domain::FallingStalactite> stalactites;
    for (const json& layer : map.at("layers")) {
        if (layer.at("type").get<std::string>() != "objectgroup") {
            continue;
        }
        for (const json& object : layer.at("objects")) {
            const std::string kind = object.value("class", object.value("type", std::string()));
            if (kind != "falling_stalactite") {
                continue;
            }
            const float width = object.value("width", 0.0F);
            const float height = object.value("height", 0.0F);
            if (width <= 0.0F || height <= 0.0F) {
                throw std::invalid_argument("a 'falling_stalactite' object must be a rectangle with a size");
            }
            stalactites.push_back(domain::FallingStalactite{domain::AABB{
                object.at("x").get<float>(), object.at("y").get<float>(), width, height}});
        }
    }
    return stalactites;
}

}  // namespace

domain::Room parseTiledRoom(const std::string& room_id, const std::string& json_text) {
    try {
        const json map = json::parse(json_text);
        return domain::Room(room_id, readTiles(map), readNeighbors(map), readCoins(map),
                            readHazards(map), readStalactites(map), readFruits(map), readEnemies(map),
                            readCheckpoints(map), readExits(map), readOrbs(map));
    } catch (const json::exception& error) {
        throw std::runtime_error("Invalid Tiled map '" + room_id + "': " + error.what());
    } catch (const std::invalid_argument& error) {
        throw std::runtime_error("Invalid Tiled map '" + room_id + "': " + error.what());
    }
}

domain::Room loadTiledRoom(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Cannot open map file: " + path);
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    return parseTiledRoom(std::filesystem::path(path).stem().string(), contents.str());
}

}  // namespace infrastructure
