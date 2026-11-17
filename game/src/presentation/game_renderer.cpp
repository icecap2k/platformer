#include "presentation/game_renderer.h"

#include <string>

#include "presentation/animation.h"
#include "presentation/decoration.h"

namespace presentation {

namespace {

// Positions inside the tileset (tiled/tiles.png), 8x8 each.
constexpr int kTileDirt = 0;
constexpr int kTileBrick = 1;
constexpr int kTileGrass = 2;
constexpr int kTileMossyBrick = 3;

// Sprite sheet (sprites/sprites.png). The player drawing is bigger than its
// 8x16 collision box because the thick outline sticks out of it.
constexpr float kPlayerSpriteWidth = 12.0F;
constexpr float kPlayerSpriteHeight = 18.0F;
constexpr float kPlayerOffsetX = -2.0F;  // sprite origin relative to the collision box
constexpr float kPlayerOffsetY = -1.0F;
constexpr float kCoinSheetY = 18.0F;  // coin frames sit below the player frames
constexpr float kCoinSize = 8.0F;

constexpr float kDecorSize = 8.0F;

constexpr Uint8 kOutlineR = 28;
constexpr Uint8 kOutlineG = 20;
constexpr Uint8 kOutlineB = 32;

// A cell outside the map counts as solid, so no outline is drawn along the
// edges of the screen (the world continues into the next room there).
bool solidOrOutside(const domain::TileMap& map, int column, int row) {
    const bool outside = column < 0 || row < 0 || column >= map.width() || row >= map.height();
    return outside || map.isSolid(column, row);
}

// Tiles get vegetation (grass, moss) on top when nothing solid is above them.
int tileSheetIndex(const domain::TileMap& map, int column, int row) {
    const bool exposed_top = !solidOrOutside(map, column, row - 1);
    if (map.tileAt(column, row) == 1) {
        return exposed_top ? kTileGrass : kTileDirt;
    }
    return exposed_top ? kTileMossyBrick : kTileBrick;
}

}  // namespace

GameRenderer::GameRenderer(SDL_Renderer* renderer, const std::string& assets_dir)
    : renderer_(renderer),
      tiles_(renderer, assets_dir + "tiled/tiles.png"),
      sprites_(renderer, assets_dir + "sprites/sprites.png"),
      decor_(renderer, assets_dir + "sprites/decor.png") {}

void GameRenderer::draw(const application::GameSession& session) {
    const std::uint64_t time_ms = SDL_GetTicks();

    drawBackground();
    drawTiles(session.map());
    drawScenery(session.map());
    drawCoins(session, time_ms);
    drawPlayer(session.player(), time_ms);
    drawHud(session, time_ms);

    SDL_RenderPresent(renderer_);
}

void GameRenderer::drawBackground() const {
    // A simple vertical gradient built from horizontal bands.
    constexpr int kBands = 30;
    for (int band = 0; band < kBands; ++band) {
        const float t = static_cast<float>(band) / static_cast<float>(kBands - 1);
        SDL_SetRenderDrawColor(renderer_, static_cast<Uint8>(20.0F + 30.0F * t),
                               static_cast<Uint8>(24.0F + 40.0F * t),
                               static_cast<Uint8>(48.0F + 70.0F * t), 255);
        const SDL_FRect rect{0.0F, static_cast<float>(band * 8), 320.0F, 8.0F};
        SDL_RenderFillRect(renderer_, &rect);
    }
}

void GameRenderer::drawTiles(const domain::TileMap& map) const {
    const float size = static_cast<float>(map.tileSize());
    for (int row = 0; row < map.height(); ++row) {
        for (int column = 0; column < map.width(); ++column) {
            if (!map.isSolid(column, row)) {
                continue;
            }
            const SDL_FRect source{static_cast<float>(tileSheetIndex(map, column, row)) * size,
                                   0.0F, size, size};
            const SDL_FRect destination{static_cast<float>(column) * size,
                                        static_cast<float>(row) * size, size, size};
            tiles_.draw(renderer_, source, destination);

            // Thick dark outline along every edge that touches open air.
            SDL_SetRenderDrawColor(renderer_, kOutlineR, kOutlineG, kOutlineB, 255);
            const float x = destination.x;
            const float y = destination.y;
            if (!solidOrOutside(map, column, row - 1)) {
                const SDL_FRect edge{x, y, size, 1.0F};
                SDL_RenderFillRect(renderer_, &edge);
            }
            if (!solidOrOutside(map, column, row + 1)) {
                const SDL_FRect edge{x, y + size - 1.0F, size, 1.0F};
                SDL_RenderFillRect(renderer_, &edge);
            }
            if (!solidOrOutside(map, column - 1, row)) {
                const SDL_FRect edge{x, y, 1.0F, size};
                SDL_RenderFillRect(renderer_, &edge);
            }
            if (!solidOrOutside(map, column + 1, row)) {
                const SDL_FRect edge{x + size - 1.0F, y, 1.0F, size};
                SDL_RenderFillRect(renderer_, &edge);
            }
        }
    }
}

// Vegetation growing out of the tiles: it sticks out of the solid area, on
// top of tiles and hanging below them.
void GameRenderer::drawScenery(const domain::TileMap& map) const {
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
                }
            }
        }
    }
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
    const PlayerPose pose = playerPose(player, time_ms);

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
