#include "presentation/game_renderer.h"

#include <algorithm>
#include <string>

#include "presentation/animation.h"
#include "presentation/autotile.h"
#include "presentation/decoration.h"
#include "presentation/drip.h"
#include "presentation/firefly.h"

namespace presentation {

namespace {

// Sprite sheet (sprites/sprites.png). The player drawing is bigger than its
// 8x16 collision box because the thick outline sticks out of it.
constexpr float kPlayerSpriteWidth = 12.0F;
constexpr float kPlayerSpriteHeight = 18.0F;
constexpr float kPlayerOffsetX = -2.0F;  // sprite origin relative to the collision box
// The sprite is aligned by its feet (a crouching player has a box half as tall): its
// last row, the outline under the soles, is this many pixels below the box's bottom.
constexpr float kPlayerSoleOverhang = 1.0F;
constexpr float kCoinSheetY = 18.0F;  // coin frames sit below the player frames
constexpr float kCoinSize = 8.0F;
constexpr float kHeartSheetY = 26.0F;  // heart icons (full, then empty) sit below the coins
constexpr float kHeartSize = 8.0F;
constexpr float kFruitSheetY = 34.0F;   // the glowing berry frames sit below the hearts
constexpr float kFruitFrameSize = 16.0F;  // the berry is 8x8, in the middle of its halo

constexpr float kDecorSize = 8.0F;

// A cell outside the map counts as filled, so no outline is drawn along the
// edges of the screen (the world continues into the next room there). Inside the
// map, `face` is the face of that cell that touches the tile being drawn: a ramp
// only fills some of its faces.
bool coversOrOutside(const domain::TileMap& map, int column, int row, domain::Side face) {
    const bool outside = column < 0 || row < 0 || column >= map.width() || row >= map.height();
    return outside || map.blocksFace(column, row, face);
}

// Which part of the rock sheet (sprites/rocks.png) to draw for the tile at
// (column, row): the column depends on which sides touch open air, the row on
// the material and a position-based texture variant (ramps have their own rows).
SDL_FRect rockSource(const domain::TileMap& map, int column, int row, float size) {
    using domain::Side;
    const bool north_open = !coversOrOutside(map, column, row - 1, Side::South);
    const bool south_open = !coversOrOutside(map, column, row + 1, Side::North);
    const bool east_open = !coversOrOutside(map, column + 1, row, Side::West);
    const bool west_open = !coversOrOutside(map, column - 1, row, Side::East);

    int mask = exposureMask(north_open, south_open, east_open, west_open);
    int sheet_row = rockSheetRow(map.tileAt(column, row), column, row);
    if (map.shape(column, row) == domain::TileShape::SlopeRight) {
        mask = slopeMask(south_open, east_open);
        sheet_row = kSlopeRowRight;
    } else if (map.shape(column, row) == domain::TileShape::SlopeLeft) {
        mask = slopeMask(south_open, west_open);
        sheet_row = kSlopeRowLeft;
    }
    return SDL_FRect{static_cast<float>(mask) * size, static_cast<float>(sheet_row) * size, size, size};
}

}  // namespace

GameRenderer::GameRenderer(SDL_Renderer* renderer, const std::string& assets_dir)
    : renderer_(renderer),
      rocks_(renderer, assets_dir + "sprites/rocks.png"),
      sprites_(renderer, assets_dir + "sprites/sprites.png"),
      decor_(renderer, assets_dir + "sprites/decor.png"),
      hazards_(renderer, assets_dir + "sprites/hazards.png"),
      enemies_(renderer, assets_dir + "sprites/enemies.png"),
      fireflies_(renderer, assets_dir + "sprites/fireflies.png"),
      checkpoints_(renderer, assets_dir + "sprites/checkpoints.png"),
      cave_(renderer, assets_dir + "background/cave.png") {}

void GameRenderer::draw(const application::GameSession* session, const application::GameFlow& flow) {
    const std::uint64_t time_ms = SDL_GetTicks();

    if (flow.screen() == application::Screen::Title || session == nullptr) {
        drawCave();
        drawMenu(flow, "PLATFORMER", 3.0F, 60.0F);
        drawText("ARROWS + ENTER      M: MUTE", 160.0F, 226.0F, 1.0F, SDL_Color{130, 130, 170, 255});
        SDL_RenderPresent(renderer_);
        return;
    }

    drawWorld(*session, time_ms);
    if (flow.screen() == application::Screen::Paused) {
        // Dim the frozen game and put the menu on top.
        SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer_, 8, 6, 20, 170);
        SDL_FRect whole{0.0F, 0.0F, 320.0F, 240.0F};
        SDL_RenderFillRect(renderer_, &whole);
        drawMenu(flow, "PAUSED", 3.0F, 70.0F);
        drawText("P OR ESC: RESUME", 160.0F, 226.0F, 1.0F, SDL_Color{130, 130, 170, 255});
    }
    SDL_RenderPresent(renderer_);
}

void GameRenderer::drawWorld(const application::GameSession& session, std::uint64_t time_ms) {

    drawCave();
    drawTiles(session.map());
    drawScenery(session.map(), time_ms);
    drawFireflies(session, time_ms);
    drawHazards(session);
    drawCheckpoints(session, time_ms);
    drawEnemies(session, time_ms);
    drawFruits(session, time_ms);
    drawCoins(session, time_ms);
    drawPlayer(session.player(), time_ms);
    drawHud(session, time_ms);
}

// Text from SDL's built-in 8x8 font, centred on `centre_x`; `scale` enlarges it.
void GameRenderer::drawText(const std::string& text, float centre_x, float y, float scale, SDL_Color colour) const {
    SDL_SetRenderScale(renderer_, scale, scale);
    SDL_SetRenderDrawColor(renderer_, colour.r, colour.g, colour.b, colour.a);
    const float width = static_cast<float>(text.size()) * 8.0F;
    SDL_RenderDebugText(renderer_, (centre_x - width * scale * 0.5F) / scale, y / scale, text.c_str());
    SDL_SetRenderScale(renderer_, 1.0F, 1.0F);
}

// A heading and the items of the flow's menu below it; the selected one is bracketed and brighter.
void GameRenderer::drawMenu(const application::GameFlow& flow, const std::string& heading, float heading_scale,
                            float top) const {
    drawText(heading, 160.0F + 2.0F, top + 2.0F, heading_scale, SDL_Color{20, 16, 40, 255});  // shadow
    drawText(heading, 160.0F, top, heading_scale, SDL_Color{190, 255, 230, 255});

    float y = top + 8.0F * heading_scale + 24.0F;
    const auto items = flow.items();
    for (std::size_t index = 0; index < items.size(); ++index) {
        const bool selected = static_cast<int>(index) == flow.selected();
        const std::string text = selected ? "> " + application::label(items[index]) + " <"
                                          : application::label(items[index]);
        drawText(text, 160.0F, y, 1.0F, selected ? SDL_Color{255, 232, 130, 255} : SDL_Color{150, 150, 190, 255});
        y += 14.0F;
    }
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
            if (!map.isGround(column, row)) {  // blocks and ramps
                continue;
            }
            const SDL_FRect destination{static_cast<float>(column) * size,
                                        static_cast<float>(row) * size, size, size};
            rocks_.draw(renderer_, rockSource(map, column, row, size), destination);
        }
    }
}

// Vegetation growing out of the tiles: it sticks out of the solid area, on
// top of blocks and hanging below them (never on ramps).
void GameRenderer::drawScenery(const domain::TileMap& map, std::uint64_t time_ms) const {
    const float size = static_cast<float>(map.tileSize());
    for (int row = 0; row < map.height(); ++row) {
        for (int column = 0; column < map.width(); ++column) {
            if (!map.isSolid(column, row)) {
                continue;
            }
            const float x = static_cast<float>(column) * size;
            const float y = static_cast<float>(row) * size;

            if (row > 0 && !map.isGround(column, row - 1)) {
                const int frame = standingDecoration(column, row);
                if (frame != kNoDecoration) {
                    decor_.draw(renderer_, SDL_FRect{static_cast<float>(frame) * kDecorSize, 0.0F, kDecorSize, kDecorSize},
                                SDL_FRect{x, y - size + 1.0F, kDecorSize, kDecorSize});
                }
            }
            if (row + 1 < map.height() && !map.isGround(column, row + 1)) {
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
    while (ground_row < map.height() && !map.isGround(column, ground_row)) {
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

// Checkpoints: a dark crystal lantern until touched; the active one glows (two frames that pulse).
void GameRenderer::drawCheckpoints(const application::GameSession& session, std::uint64_t time_ms) const {
    constexpr float kSprite = 8.0F;
    const domain::Room& room = session.currentRoom();
    for (std::size_t index = 0; index < room.checkpoints().size(); ++index) {
        const domain::AABB box = room.checkpoints()[index].bounds();
        const bool active = session.lastCheckpoint() &&
                            *session.lastCheckpoint() == std::make_pair(room.id(), index);
        const int frame = active ? checkpointFrame(time_ms) : 0;
        checkpoints_.draw(renderer_, SDL_FRect{static_cast<float>(frame) * kSprite, 0.0F, kSprite, kSprite},
                          SDL_FRect{box.x, box.bottom() - kSprite, kSprite, kSprite});
    }
}

// Fireflies: pure decoration, behind everything that matters (hazards, enemies, player).
void GameRenderer::drawFireflies(const application::GameSession& session, std::uint64_t time_ms) {
    constexpr float kSprite = 8.0F;
    if (firefly_room_ != session.currentRoom().id()) {
        firefly_room_ = session.currentRoom().id();
        firefly_routes_ = fireflyRoutes(firefly_room_, session.map());
    }
    for (const FireflyRoute& route : firefly_routes_) {
        const Firefly firefly = fireflyAt(route, time_ms);
        fireflies_.draw(renderer_, SDL_FRect{static_cast<float>(firefly.frame) * kSprite, 0.0F, kSprite, kSprite},
                        SDL_FRect{firefly.x, firefly.y, kSprite, kSprite}, firefly.facing_left);
    }
}

// Spikes: the hazard's rectangle is the hitbox, and the 8x8 spike sprite is
// repeated along it. The sprite is drawn one pixel lower than the rectangle's
// bottom edge: its last row is a dark line that merges with the floor's top
// outline, so the teeth look embedded in the floor.
void GameRenderer::drawHazards(const application::GameSession& session) const {
    constexpr float kSprite = 8.0F;
    constexpr float kEmbed = 1.0F;  // how many pixels the sprite sinks into the floor
    const domain::Room& room = session.currentRoom();
    for (const domain::Hazard& hazard : room.hazards()) {
        const domain::AABB& box = hazard.bounds;
        for (float x = box.x; x < box.right(); x += kSprite) {
            const float width = std::min(kSprite, box.right() - x);
            hazards_.draw(renderer_, SDL_FRect{0.0F, 0.0F, width, kSprite},
                          SDL_FRect{x, box.bottom() - kSprite + kEmbed, width, kSprite});
        }
    }

    // Stalactite traps (frame 1 of the hazard sheet): they shake as a warning
    // (a one pixel wobble) and then fall; once shattered they are gone.
    for (std::size_t index = 0; index < room.stalactites().size(); ++index) {
        const domain::StalactiteState& state = session.stalactiteTraps().state(room.id(), index);
        if (state.phase == domain::StalactitePhase::Gone) {
            continue;
        }
        const domain::AABB box = domain::currentBounds(room.stalactites()[index], state);
        const float wobble = (state.phase == domain::StalactitePhase::Shaking)
                                 ? ((state.shake_steps / 2) % 2 == 0 ? -1.0F : 1.0F)
                                 : 0.0F;
        hazards_.draw(renderer_, SDL_FRect{kSprite, 0.0F, kSprite, kSprite},
                      SDL_FRect{box.x + wobble, box.y, kSprite, kSprite});
    }
}

// The enemies still alive: the beetle faces the way it walks (the sheet draws it facing right).
void GameRenderer::drawEnemies(const application::GameSession& session, std::uint64_t time_ms) const {
    constexpr float kSprite = 8.0F;
    const domain::Room& room = session.currentRoom();
    for (std::size_t index = 0; index < room.enemies().size(); ++index) {
        const domain::EnemyState& state = session.enemyPatrols().state(room.id(), index);
        const domain::AABB box = domain::currentBounds(room.enemies()[index], state);
        // The sprite is 8x8: sit it on the bottom of the hitbox.
        const SDL_FRect destination{box.x, box.bottom() - kSprite, kSprite, kSprite};
        if (!state.alive) {
            // Frame 2: squashed flat. It blinks for its last third of a second, then it is gone.
            if (domain::isSquashedBody(state) && isVisibleWhileFading(domain::kSquashSteps - state.dead_steps)) {
                enemies_.draw(renderer_, SDL_FRect{2.0F * kSprite, 0.0F, kSprite, kSprite}, destination,
                              state.direction < 0);
            }
            continue;
        }
        enemies_.draw(renderer_, SDL_FRect{static_cast<float>(enemyFrame(time_ms)) * kSprite, 0.0F, kSprite, kSprite},
                      destination, state.direction < 0);
    }
}

// The healing fruits of the room that nobody has eaten yet: a berry that glows.
void GameRenderer::drawFruits(const application::GameSession& session, std::uint64_t time_ms) const {
    const domain::Room& room = session.currentRoom();
    const SDL_FRect source{static_cast<float>(fruitFrame(time_ms)) * kFruitFrameSize, kFruitSheetY,
                           kFruitFrameSize, kFruitFrameSize};
    const float bob = static_cast<float>(fruitBob(time_ms));
    for (std::size_t index = 0; index < room.fruits().size(); ++index) {
        if (session.eatenFruits().isCollected(room.id(), index)) {
            continue;
        }
        const domain::AABB box = room.fruits()[index].bounds();
        // The frame is bigger than the fruit: centre it, so the halo spills around the hitbox.
        const float margin = (kFruitFrameSize - box.width) * 0.5F;
        sprites_.draw(renderer_, source,
                      SDL_FRect{box.x - margin, box.y - margin + bob, kFruitFrameSize, kFruitFrameSize});
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
    const SDL_FRect destination{box.x + kPlayerOffsetX, box.bottom() - kPlayerSpriteHeight + kPlayerSoleOverhang,
                                kPlayerSpriteWidth, kPlayerSpriteHeight};
    if (!isVisibleWhileInvulnerable(player.invulnerable_steps)) {
        return;  // blinking after a hit
    }
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

    // Hearts, top right: full ones, then empty ones for the hearts lost.
    constexpr float kHeartGap = 1.0F;
    const float first_heart_x = 320.0F - 6.0F - static_cast<float>(domain::kMaxHealth) * (kHeartSize + kHeartGap) + kHeartGap;
    for (int heart = 0; heart < domain::kMaxHealth; ++heart) {
        const float column = heart < session.player().health ? 0.0F : 1.0F;
        sprites_.draw(renderer_, SDL_FRect{column * kHeartSize, kHeartSheetY, kHeartSize, kHeartSize},
                      SDL_FRect{first_heart_x + static_cast<float>(heart) * (kHeartSize + kHeartGap), 5.0F,
                                kHeartSize, kHeartSize});
    }
}

}  // namespace presentation
