#include "presentation/game_renderer.h"

#include <string>

namespace presentation {

namespace {

void setTileColor(SDL_Renderer* renderer, int tile_id) {
    if (tile_id == 1) {
        SDL_SetRenderDrawColor(renderer, 110, 80, 55, 255);  // ground
    } else {
        SDL_SetRenderDrawColor(renderer, 170, 90, 70, 255);  // anything else: brick
    }
}

void drawTiles(SDL_Renderer* renderer, const domain::TileMap& map) {
    const float size = static_cast<float>(map.tileSize());
    for (int row = 0; row < map.height(); ++row) {
        for (int column = 0; column < map.width(); ++column) {
            const int tile_id = map.tileAt(column, row);
            if (tile_id == 0) {
                continue;
            }
            setTileColor(renderer, tile_id);
            const SDL_FRect rect{static_cast<float>(column) * size, static_cast<float>(row) * size,
                                 size, size};
            SDL_RenderFillRect(renderer, &rect);
        }
    }
}

void drawCoins(SDL_Renderer* renderer, const application::GameSession& session) {
    const domain::Room& room = session.currentRoom();
    for (std::size_t index = 0; index < room.coins().size(); ++index) {
        if (session.collectedCoins().isCollected(room.id(), index)) {
            continue;
        }
        const domain::AABB box = room.coins()[index].bounds();
        const SDL_FRect outer{box.x, box.y, box.width, box.height};
        SDL_SetRenderDrawColor(renderer, 255, 160, 30, 255);
        SDL_RenderFillRect(renderer, &outer);

        const SDL_FRect inner{box.x + 2.0F, box.y + 2.0F, box.width - 4.0F, box.height - 4.0F};
        SDL_SetRenderDrawColor(renderer, 255, 225, 120, 255);
        SDL_RenderFillRect(renderer, &inner);
    }
}

void drawHud(SDL_Renderer* renderer, const application::GameSession& session) {
    const std::string text = "COINS " + std::to_string(session.collectedCoins().count()) + "/" +
                             std::to_string(session.totalCoins());
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(renderer, 6.0F, 6.0F, text.c_str());
}

}  // namespace

void drawGame(SDL_Renderer* renderer, const application::GameSession& session) {
    // Background.
    SDL_SetRenderDrawColor(renderer, 20, 24, 48, 255);
    SDL_RenderClear(renderer);

    drawTiles(renderer, session.map());
    drawCoins(renderer, session);

    // Player: a plain rectangle until we have sprites (E7).
    const domain::AABB box = session.player().bounds();
    SDL_SetRenderDrawColor(renderer, 240, 200, 60, 255);
    const SDL_FRect rect{box.x, box.y, box.width, box.height};
    SDL_RenderFillRect(renderer, &rect);

    drawHud(renderer, session);

    SDL_RenderPresent(renderer);
}

}  // namespace presentation
