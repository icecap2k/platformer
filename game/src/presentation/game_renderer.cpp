#include "presentation/game_renderer.h"

#include <string>

#include "presentation/animation.h"
#include "presentation/autotile.h"
#include "presentation/decoration.h"
#include "presentation/drip.h"

namespace presentation {

namespace {

// Sprite sheet (sprites/sprites.png). The player drawing is bigger than its
// 8x16 collision box because the thick outline sticks out of it.
constexpr float kPlayerSpriteWidth = 12.0F;
constexpr float kPlayerSpriteHeight = 18.0F;
constexpr float kPlayerOffsetX = -2.0F;  // sprite origin relative to the collision box
constexpr float kPlayerOffsetY = -1.0F;
constexpr float kCoinSheetY = 18.0F;  // coin frames sit below the player frames
constexpr float kCoinSize = 8.0F;

constexpr float kDecorSize = 8.0F;

// A cell outside the map counts as solid, so no outline is drawn along the
// edges of the screen (the world continues into the next room there).
bool solidOrOutside(const domain::TileMap& map, int column, int row) {
    const bool outside = column < 0 || row < 0 || column >= map.width() || row >= map.height();
    return outside || map.isSolid(column, row);
}

// Which part of the rock sheet (sprites/rocks.png) to draw for the tile at
// (column, row): the column depends on which sides touch open air, the row on
// the material and a position-based texture variant.
SDL_FRect rockSource(const domain::TileMap& map, int column, int row, float size) {
    const int mask = exposureMask(!solidOrOutside(map, column, row - 1),
                                  !solidOrOutside(map, column, row + 1),
                                  !solidOrOutside(map, column + 1, row),
                                  !solidOrOutside(map, column - 1, row));
    const int sheet_row = rockSheetRow(map.tileAt(column, row), column, row);
    return SDL_FRect{static_cast<float>(mask) * size, static_cast<float>(sheet_row) * size, size, size};
}

}  // namespace

GameRenderer::GameRenderer(SDL_Renderer* renderer, const std::string& assets_dir)
    : renderer_(renderer),
      rocks_(renderer, assets_dir + "sprites/rocks.png"),
      sprites_(renderer, assets_dir + "sprites/sprites.png"),
      decor_(renderer, assets_dir + "sprites/decor.png"),
      cave_(renderer, assets_dir + "background/cave.png") {}

void GameRenderer::draw(const application::GameSession& session) {
    const std::uint64_t time_ms = SDL_GetTicks();

    drawCave();
    drawTiles(session.map());
    drawScenery(session.map(), time_ms);
    drawCoins(session, time_ms);
    drawPlayer(session.player(), time_ms);
    drawHud(session, time_ms);

    SDL_RenderPresent(renderer_);
}

// The cave behind the level: one still picture of the whole screen.
void GameRenderer::drawCave() const {
    cave_.draw(renderer_, SDL_FRect{0.0F, 0.0F, 320.0F, 240.0F}, SDL_FRect{0.0F, 0.0F, 320.0F, 240.0F});
}

// The outline is part of the rock art (it follows the pointed tips), so there is
// nothing to draw besides the tile itself.
void GameRenderer::drawTiles(const domain::TileMap& map) const {
    const float size = static_cast<float>(map.tileSize());
    for (int row = 0; row < map.height(); ++row) {
        for (int column = 0; column < map.width(); ++column) {
            if (!map.isSolid(column, row)) {
                continue;
            }
            const SDL_FRect destination{static_cast<float>(column) * size,
                                        static_cast<float>(row) * size, size, size};
            rocks_.draw(renderer_, rockSource(map, column, row, size), destination);
        }
    }
}

// Vegetation growing out of the tiles: it sticks out of the solid area, on
// top of tiles and hanging below them.
void GameRenderer::drawScenery(const domain::TileMap& map, std::uint64_t time_ms) const {
    const float size = static_cast<float>(map.tileSize());
    for (int row = 0; row < map.height(); ++row) {
        for (int column = 0; column < map.width(); ++column) {
            if (!map.isSolid(column, row)) {
                continue;
            }
            const float x = static_cast<float>(column) * size;
            const float y = static_cast<float>(row) * size;

            if (row > 0 && !map.isSolid(column, row - 1)) {
                const int frame = standingDecoration(column, row);
                if (frame != kNoDecoration) {
                    decor_.draw(renderer_, SDL_FRect{static_cast<float>(frame) * kDecorSize, 0.0F, kDecorSize, kDecorSize},
                                SDL_FRect{x, y - size + 1.0F, kDecorSize, kDecorSize});
                }
            }
            if (row + 1 < map.height() && !map.isSolid(column, row + 1)) {
                const int frame = hangingDecoration(column, row);
                if (frame != kNoDecoration) {
                    decor_.draw(renderer_, SDL_FRect{static_cast<float>(frame) * kDecorSize, 0.0F, kDecorSize, kDecorSize},
                                SDL_FRect{x, y + size - 1.0F, kDecorSize, kDecorSize});
                    if (frame == kDecorStalactite) {
                        drawDrip(map, column, row, time_ms);
                    }
                }
            }
        }
    }
}

// The water drop of the stalactite hanging from the tile at (column, row): it
// forms at the tip, falls until it meets the first solid tile below (or the
// bottom of the screen) and splashes there. Only decoration: nothing in the
// game reacts to it.
void GameRenderer::drawDrip(const domain::TileMap& map, int column, int row,
                            std::uint64_t time_ms) const {
    constexpr float kDropTopInSprite = 5.0F;  // where the drop starts, inside the stalactite frame
    constexpr float kDropHeight = 2.0F;

    const float size = static_cast<float>(map.tileSize());
    int ground_row = row + 1;
    while (ground_row < map.height() && !map.isSolid(column, ground_row)) {
        ++ground_row;
    }
    const float ground_y = static_cast<float>(ground_row) * size;
    const float sprite_y = static_cast<float>(row) * size + size - 1.0F;
    const float fall_distance = ground_y - (sprite_y + kDropTopInSprite + kDropHeight);

    const Drip drip = dripAt(column, row, fall_distance, time_ms);
    if (drip.phase == DripPhase::None) {
        return;
    }
    const float x = static_cast<float>(column) * size;
    const SDL_FRect source{static_cast<float>(drip.frame) * kDecorSize, 0.0F, kDecorSize, kDecorSize};
    float y = sprite_y;  // forming: right under the stalactite's tip
    if (drip.phase == DripPhase::Falling) {
        y += drip.fall_offset;
    } else if (drip.phase == DripPhase::Splash) {
        y = ground_y - kDecorSize;  // the splash frames end in their bottom row
    }
    decor_.draw(renderer_, source, SDL_FRect{x, y, kDecorSize, kDecorSize});
}

void GameRenderer::drawCoins(const application::GameSession& session, std::uint64_t time_ms) const {
    const domain::Room& room = session.currentRoom();
    const SDL_FRect source{static_cast<float>(coinFrame(time_ms)) * kCoinSize, kCoinSheetY,
                           kCoinSize, kCoinSize};

    for (std::size_t index = 0; index < room.coins().size(); ++index) {
        if (session.collectedCoins().isCollected(room.id(), index)) {
            continue;
        }
        const domain::AABB box = room.coins()[index].bounds();
        sprites_.draw(renderer_, source, SDL_FRect{box.x, box.y, box.width, box.height});
    }
}

void GameRenderer::drawPlayer(const domain::Player& player, std::uint64_t time_ms) {
    facing_left_ = facesLeft(player, facing_left_);
    if (player.on_ground && !was_on_ground_) {
        landed_at_ms_ = time_ms;
        has_landed_ = true;
    }
    was_on_ground_ = player.on_ground;
    const std::uint64_t since_landing = has_landed_ ? time_ms - landed_at_ms_ : kNotLanding;
    const PlayerPose pose = playerPose(player, time_ms, since_landing);

    const SDL_FRect source{static_cast<float>(static_cast<int>(pose)) * kPlayerSpriteWidth, 0.0F,
                           kPlayerSpriteWidth, kPlayerSpriteHeight};
    const domain::AABB box = player.bounds();
    const SDL_FRect destination{box.x + kPlayerOffsetX, box.y + kPlayerOffsetY, kPlayerSpriteWidth,
                                kPlayerSpriteHeight};
    sprites_.draw(renderer_, source, destination, facing_left_);
}

void GameRenderer::drawHud(const application::GameSession& session, std::uint64_t time_ms) const {
    const SDL_FRect icon_source{static_cast<float>(coinFrame(time_ms)) * kCoinSize, kCoinSheetY,
                                kCoinSize, kCoinSize};
    sprites_.draw(renderer_, icon_source, SDL_FRect{6.0F, 5.0F, kCoinSize, kCoinSize});

    const std::string text = std::to_string(session.collectedCoins().count()) + "/" +
                             std::to_string(session.totalCoins());
    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
    SDL_RenderDebugText(renderer_, 18.0F, 5.0F, text.c_str());
}

}  // namespace presentation
