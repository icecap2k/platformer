#include "level_tools/text_level.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace level_tools {

namespace {

using nlohmann::json;
using domain::Side;

[[noreturn]] void failAt(int line, const std::string& message) {
    throw std::runtime_error("line " + std::to_string(line) + ": " + message);
}

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = text.find_last_not_of(" \t\r");
    return text.substr(first, last - first + 1);
}

const std::map<std::string, Side>& sideNames() {
    static const std::map<std::string, Side> names = {
        {"north", Side::North}, {"south", Side::South}, {"east", Side::East}, {"west", Side::West}};
    return names;
}

// Enemies are two tiles big: centred on the symbol's tile and standing on
// its bottom edge.
domain::AABB largeItemAt(int column, int row, int tile_size) {
    const float tile = static_cast<float>(tile_size);
    return domain::AABB{static_cast<float>(column) * tile - tile * 0.5F, static_cast<float>(row - 1) * tile,
                        tile * 2.0F, tile * 2.0F};
}

// The tile a large item is anchored to: the one under the middle of its bottom edge.
int anchorColumn(float x, float width, int tile_size) {
    return static_cast<int>(std::floor((x + width * 0.5F) / static_cast<float>(tile_size)));
}
int anchorRow(float y, float height, int tile_size) {
    return static_cast<int>(std::floor((y + height - 0.5F) / static_cast<float>(tile_size)));
}

}  // namespace

TextLevel parseTextLevel(const std::string& text) {
    std::vector<std::string> lines;
    {
        std::istringstream stream(text);
        std::string line;
        while (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            lines.push_back(line);
        }
    }

    TextLevel level;
    std::size_t separator = lines.size();
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (trim(lines[i]) == "---") {
            separator = i;
            break;
        }
    }
    if (separator == lines.size()) {
        throw std::runtime_error("missing the '---' line that separates header and grid");
    }

    // ---- header ----
    for (std::size_t i = 0; i < separator; ++i) {
        const int line_number = static_cast<int>(i) + 1;
        const std::string line = trim(lines[i]);
        if (line.empty() || line.front() == '#') {
            continue;
        }
        const auto colon = line.find(':');
        if (colon == std::string::npos) {
            failAt(line_number, "expected 'key: value', got '" + line + "'");
        }
        const std::string key = trim(line.substr(0, colon));
        const std::string value = trim(line.substr(colon + 1));

        if (key == "tile_size") {
            try {
                level.tile_size = std::stoi(value);
            } catch (const std::exception&) {
                failAt(line_number, "tile_size must be a whole number, got '" + value + "'");
            }
            if (level.tile_size <= 0) {
                failAt(line_number, "tile_size must be positive");
            }
        } else if (const auto side = sideNames().find(key); side != sideNames().end()) {
            if (value.empty()) {
                failAt(line_number, "'" + key + "' needs a room id");
            }
            level.neighbors[side->second] = value;
        } else {
            failAt(line_number, "unknown header key '" + key +
                                    "' (use north, south, east, west or tile_size)");
        }
    }

    // ---- grid ----
    std::vector<std::string> rows(lines.begin() + static_cast<std::ptrdiff_t>(separator) + 1,
                                  lines.end());
    while (!rows.empty() && trim(rows.back()).empty()) {
        rows.pop_back();
    }
    if (rows.empty()) {
        throw std::runtime_error("the grid below '---' is empty");
    }

    level.height = static_cast<int>(rows.size());
    level.width = static_cast<int>(rows.front().size());
    level.tiles.assign(static_cast<std::size_t>(level.width * level.height), 0);

    for (int row = 0; row < level.height; ++row) {
        const int line_number = static_cast<int>(separator) + 2 + row;
        const std::string& cells = rows[static_cast<std::size_t>(row)];
        if (static_cast<int>(cells.size()) != level.width) {
            failAt(line_number, "this row has " + std::to_string(cells.size()) +
                                    " characters but the first one has " +
                                    std::to_string(level.width));
        }
        for (int column = 0; column < level.width; ++column) {
            int tile = 0;
            switch (cells[static_cast<std::size_t>(column)]) {
                case '.':
                    break;
                case '#':
                    tile = kTileGround;
                    break;
                case 'B':
                    tile = kTileBrick;
                    break;
                case '3':
                    tile = kTileThree;
                    break;
                case '4':
                    tile = kTileFour;
                    break;
                case '/':
                    tile = domain::kTileSlopeRight;
                    break;
                case '\\':
                    tile = domain::kTileSlopeLeft;
                    break;
                case '^': {
                    const float size = static_cast<float>(level.tile_size);
                    const float inset = size * kSpikeInsetFraction;
                    level.hazards.push_back(domain::Hazard{domain::AABB{
                        static_cast<float>(column) * size, static_cast<float>(row) * size + inset,
                        size, size - inset}});
                    break;
                }
                case 'V': {
                    const float size = static_cast<float>(level.tile_size);
                    level.stalactites.push_back(domain::FallingStalactite{domain::AABB{
                        static_cast<float>(column) * size, static_cast<float>(row) * size, size, size}});
                    break;
                }
                case 'X': {
                    const float size = static_cast<float>(level.tile_size);
                    level.exits.push_back(domain::Exit{static_cast<float>(column) * size,
                                                       static_cast<float>(row) * size, size});
                    break;
                }
                case 'K': {
                    const float size = static_cast<float>(level.tile_size);
                    level.checkpoints.push_back(domain::Checkpoint{static_cast<float>(column) * size,
                                                                 static_cast<float>(row) * size, size});
                    break;
                }
                case 'E': {
                    const domain::AABB box = largeItemAt(column, row, level.tile_size);
                    level.enemies.push_back(domain::Enemy{box});
                    break;
                }
                case 'F': {
                    const float size = static_cast<float>(level.tile_size);
                    level.fruits.push_back(domain::Fruit{static_cast<float>(column) * size,
                                                       static_cast<float>(row) * size, size});
                    break;
                }
                case 'C': {
                    const float size = static_cast<float>(level.tile_size);
                    level.coins.push_back(domain::Coin{static_cast<float>(column) * size,
                                                       static_cast<float>(row) * size, size});
                    break;
                }
                default:
                    failAt(line_number, std::string("unknown character '") +
                                            cells[static_cast<std::size_t>(column)] +
                                            "' (use . # B 3 4 / \\ ^ V C F E K X)");
            }
            level.tiles[static_cast<std::size_t>(row * level.width + column)] = tile;
        }
    }
    return level;
}

domain::Room toRoom(const domain::RoomId& id, const TextLevel& level) {
    return domain::Room(id, domain::TileMap(level.width, level.height, level.tile_size, level.tiles),
                        level.neighbors, level.coins, level.hazards, level.stalactites,
                        level.fruits, level.enemies, level.checkpoints,
                        level.exits);
}

std::string toTiledJson(const TextLevel& level) {
    json properties = json::array();
    for (const auto& [name, side] : sideNames()) {
        const auto neighbor = level.neighbors.find(side);
        if (neighbor != level.neighbors.end()) {
            properties.push_back({{"name", name}, {"type", "string"}, {"value", neighbor->second}});
        }
    }

    json objects = json::array();
    int object_id = 1;
    for (const domain::Exit& exit : level.exits) {
        objects.push_back({{"id", object_id++},
                           {"name", "exit"},
                           {"type", ""},
                           {"class", "exit"},
                           {"x", exit.x},
                           {"y", exit.y},
                           {"width", exit.size},
                           {"height", exit.size},
                           {"rotation", 0},
                           {"visible", true}});
    }
    for (const domain::Checkpoint& checkpoint : level.checkpoints) {
        objects.push_back({{"id", object_id++},
                           {"name", "checkpoint"},
                           {"type", ""},
                           {"class", "checkpoint"},
                           {"x", checkpoint.x},
                           {"y", checkpoint.y},
                           {"width", checkpoint.size},
                           {"height", checkpoint.size},
                           {"rotation", 0},
                           {"visible", true}});
    }
    for (const domain::Enemy& enemy : level.enemies) {
        objects.push_back({{"id", object_id++},
                           {"name", "walker"},
                           {"type", ""},
                           {"class", "walker"},
                           {"x", enemy.bounds.x},
                           {"y", enemy.bounds.y},
                           {"width", enemy.bounds.width},
                           {"height", enemy.bounds.height},
                           {"rotation", 0},
                           {"visible", true}});
    }
    for (const domain::Fruit& fruit : level.fruits) {
        objects.push_back({{"id", object_id++},
                           {"name", "fruit"},
                           {"type", ""},
                           {"class", "fruit"},
                           {"x", fruit.x},
                           {"y", fruit.y},
                           {"width", fruit.size},
                           {"height", fruit.size},
                           {"rotation", 0},
                           {"visible", true}});
    }
    for (const domain::Coin& coin : level.coins) {
        objects.push_back({{"id", object_id++},
                           {"name", "coin"},
                           {"type", ""},
                           {"class", "coin"},
                           {"x", coin.x},
                           {"y", coin.y},
                           {"width", coin.size},
                           {"height", coin.size},
                           {"rotation", 0},
                           {"visible", true}});
    }

    for (const domain::Hazard& hazard : level.hazards) {
        objects.push_back({{"id", object_id++},
                           {"name", "spikes"},
                           {"type", ""},
                           {"class", "spikes"},
                           {"x", hazard.bounds.x},
                           {"y", hazard.bounds.y},
                           {"width", hazard.bounds.width},
                           {"height", hazard.bounds.height},
                           {"rotation", 0},
                           {"visible", true}});
    }

    for (const domain::FallingStalactite& stalactite : level.stalactites) {
        objects.push_back({{"id", object_id++},
                           {"name", "stalactite"},
                           {"type", ""},
                           {"class", "falling_stalactite"},
                           {"x", stalactite.bounds.x},
                           {"y", stalactite.bounds.y},
                           {"width", stalactite.bounds.width},
                           {"height", stalactite.bounds.height},
                           {"rotation", 0},
                           {"visible", true}});
    }

    const json map = {
        {"compressionlevel", -1},
        {"width", level.width},
        {"height", level.height},
        {"tilewidth", level.tile_size},
        {"tileheight", level.tile_size},
        {"infinite", false},
        {"orientation", "orthogonal"},
        {"renderorder", "right-down"},
        {"tiledversion", "1.10.2"},
        {"version", "1.10"},
        {"type", "map"},
        {"nextlayerid", 3},
        {"nextobjectid", object_id},
        {"properties", properties},
        {"layers",
         json::array({{{"id", 1},
                       {"name", "ground"},
                       {"type", "tilelayer"},
                       {"width", level.width},
                       {"height", level.height},
                       {"x", 0},
                       {"y", 0},
                       {"opacity", 1},
                       {"visible", true},
                       {"data", level.tiles}},
                      {{"id", 2},
                       {"name", "objects"},
                       {"type", "objectgroup"},
                       {"draworder", "topdown"},
                       {"x", 0},
                       {"y", 0},
                       {"opacity", 1},
                       {"visible", true},
                       {"objects", objects}}})},
        {"tilesets",
         json::array({{{"firstgid", 1},
                       {"name", "tiles"},
                       {"image", "tiles.png"},
                       {"imagewidth", 4 * level.tile_size},
                       {"imageheight", level.tile_size},
                       {"tilewidth", level.tile_size},
                       {"tileheight", level.tile_size},
                       {"tilecount", 4},
                       {"columns", 4},
                       {"margin", 0},
                       {"spacing", 0}}})},
    };
    return map.dump(1) + "\n";
}

std::string toText(const domain::Room& room) {
    const domain::TileMap& map = room.map();
    std::vector<std::string> grid(static_cast<std::size_t>(map.height()),
                                  std::string(static_cast<std::size_t>(map.width()), '.'));

    for (int row = 0; row < map.height(); ++row) {
        for (int column = 0; column < map.width(); ++column) {
            char symbol = '.';
            switch (map.tileAt(column, row)) {
                case 0:
                    break;
                case kTileGround:
                    symbol = '#';
                    break;
                case kTileBrick:
                    symbol = 'B';
                    break;
                case kTileThree:
                    symbol = '3';
                    break;
                case kTileFour:
                    symbol = '4';
                    break;
                case domain::kTileSlopeRight:
                    symbol = '/';
                    break;
                case domain::kTileSlopeLeft:
                    symbol = '\\';
                    break;
                default:
                    throw std::runtime_error("tile id " + std::to_string(map.tileAt(column, row)) +
                                             " at column " + std::to_string(column) + ", row " +
                                             std::to_string(row) + " has no text symbol");
            }
            grid[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)] = symbol;
        }
    }

    for (const domain::Hazard& hazard : room.hazards()) {
        const float size = static_cast<float>(map.tileSize());
        const float inset = size * kSpikeInsetFraction;
        const int column = static_cast<int>(std::lround(hazard.bounds.x / size));
        const int row = static_cast<int>(std::lround((hazard.bounds.y - inset) / size));
        const int cells = static_cast<int>(std::lround(hazard.bounds.width / size));  // a wide patch = several spikes
        const bool is_tile_spike = cells >= 1 && column >= 0 && column + cells <= map.width() && row >= 0 &&
                                   row < map.height() &&
                                   hazard.bounds.x == static_cast<float>(column) * size &&
                                   hazard.bounds.y == static_cast<float>(row) * size + inset &&
                                   hazard.bounds.width == static_cast<float>(cells) * size &&
                                   hazard.bounds.height == size - inset;
        if (!is_tile_spike) {
            throw std::runtime_error("a hazard at (" + std::to_string(hazard.bounds.x) + ", " +
                                     std::to_string(hazard.bounds.y) +
                                     ") is not a tile-aligned spike, so it has no text symbol");
        }
        for (int cell_column = column; cell_column < column + cells; ++cell_column) {
            char& cell = grid[static_cast<std::size_t>(row)][static_cast<std::size_t>(cell_column)];
            if (cell != '.') {
                throw std::runtime_error("spikes at column " + std::to_string(cell_column) + ", row " +
                                         std::to_string(row) + " sit on a solid tile");
            }
            cell = '^';
        }
    }

    for (const domain::FallingStalactite& stalactite : room.stalactites()) {
        const float size = static_cast<float>(map.tileSize());
        const int column = static_cast<int>(std::lround(stalactite.bounds.x / size));
        const int row = static_cast<int>(std::lround(stalactite.bounds.y / size));
        const bool is_tile = column >= 0 && column < map.width() && row >= 0 && row < map.height() &&
                             stalactite.bounds.x == static_cast<float>(column) * size &&
                             stalactite.bounds.y == static_cast<float>(row) * size &&
                             stalactite.bounds.width == size && stalactite.bounds.height == size;
        if (!is_tile) {
            throw std::runtime_error("a stalactite trap at (" + std::to_string(stalactite.bounds.x) + ", " +
                                     std::to_string(stalactite.bounds.y) +
                                     ") is not exactly one tile, so it has no text symbol");
        }
        char& cell = grid[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)];
        if (cell != '.') {
            throw std::runtime_error("a stalactite trap at column " + std::to_string(column) + ", row " +
                                     std::to_string(row) + " sits on a solid tile or a spike");
        }
        cell = 'V';
    }

    for (const domain::Enemy& enemy : room.enemies()) {
        const int column = anchorColumn(enemy.bounds.x, enemy.bounds.width, map.tileSize());
        const int row = anchorRow(enemy.bounds.y, enemy.bounds.height, map.tileSize());
        const domain::AABB exact = largeItemAt(column, row, map.tileSize());
        const bool is_symbol = column >= 0 && column < map.width() && row >= 0 && row < map.height() &&
                               enemy.bounds.x == exact.x && enemy.bounds.y == exact.y &&
                               enemy.bounds.width == exact.width && enemy.bounds.height == exact.height;
        if (!is_symbol) {
            throw std::runtime_error("an enemy at (" + std::to_string(enemy.bounds.x) + ", " +
                                     std::to_string(enemy.bounds.y) +
                                     ") does not match the E symbol's two-tile box, so it has no text symbol");
        }
        char& cell = grid[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)];
        if (cell != '.') {
            throw std::runtime_error("an enemy at column " + std::to_string(column) + ", row " +
                                     std::to_string(row) + " sits on a solid tile or another item");
        }
        cell = 'E';
    }

    for (const domain::Exit& exit : room.exits()) {
        const float size = static_cast<float>(map.tileSize());
        const int column = std::clamp(static_cast<int>(std::lround(exit.x / size)), 0, map.width() - 1);
        const int row = std::clamp(static_cast<int>(std::lround(exit.y / size)), 0, map.height() - 1);
        char& cell = grid[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)];
        if (cell != '.') {
            throw std::runtime_error("an exit at column " + std::to_string(column) + ", row " +
                                     std::to_string(row) + " sits on a solid tile or another item");
        }
        cell = 'X';
    }

    for (const domain::Checkpoint& checkpoint : room.checkpoints()) {
        const float size = static_cast<float>(map.tileSize());
        const int column = std::clamp(static_cast<int>(std::lround(checkpoint.x / size)), 0, map.width() - 1);
        const int row = std::clamp(static_cast<int>(std::lround(checkpoint.y / size)), 0, map.height() - 1);
        char& cell = grid[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)];
        if (cell != '.') {
            throw std::runtime_error("a checkpoint at column " + std::to_string(column) + ", row " +
                                     std::to_string(row) + " sits on a solid tile or another item");
        }
        cell = 'K';
    }

    for (const domain::Fruit& fruit : room.fruits()) {
        const float size = static_cast<float>(map.tileSize());
        const int column = std::clamp(static_cast<int>(std::lround(fruit.x / size)), 0, map.width() - 1);
        const int row = std::clamp(static_cast<int>(std::lround(fruit.y / size)), 0, map.height() - 1);
        char& cell = grid[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)];
        if (cell != '.') {
            throw std::runtime_error("a fruit at column " + std::to_string(column) + ", row " +
                                     std::to_string(row) + " sits on a solid tile or another item");
        }
        cell = 'F';
    }

    for (const domain::Coin& coin : room.coins()) {
        const float size = static_cast<float>(map.tileSize());
        const int column = std::clamp(static_cast<int>(std::lround(coin.x / size)), 0, map.width() - 1);
        const int row = std::clamp(static_cast<int>(std::lround(coin.y / size)), 0, map.height() - 1);
        char& cell = grid[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)];
        if (cell != '.' && cell != 'C') {
            throw std::runtime_error("a coin at column " + std::to_string(column) + ", row " +
                                     std::to_string(row) + " sits on a solid tile");
        }
        cell = 'C';
    }

    std::ostringstream out;
    for (const auto& [name, side] : sideNames()) {
        if (const auto neighbor = room.neighbor(side)) {
            out << name << ": " << *neighbor << '\n';
        }
    }
    if (map.tileSize() != 8) {
        out << "tile_size: " << map.tileSize() << '\n';
    }
    out << "---\n";
    for (const std::string& row : grid) {
        out << row << '\n';
    }
    return out.str();
}

}  // namespace level_tools
