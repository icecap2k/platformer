#include "presentation/game_renderer.h"

#include <algorithm>
#include <string>

#include "domain/rules/damage.h"
#include "presentation/animation.h"
#include "presentation/autotile.h"
#include "presentation/decoration.h"
#include "presentation/drip.h"
#include "presentation/firefly.h"
#include "presentation/screen_shake.h"

namespace presentation {

namespace {

// The hero (hero.png) is drawn bigger than its 8x16 collision box, centred on it.
constexpr float kOrbCell = 26.0F;  // a cell of orbs.png: one row per colour, the silhouette after the frames
constexpr float kPlayerSpriteWidth = 32.0F;
constexpr float kPlayerSpriteHeight = 28.0F;
// The sprite is aligned by its feet (a crouching player has a box half as tall): its
// last row, the outline under the soles, is this many pixels below the box's bottom.
constexpr float kPlayerSoleOverhang = 1.0F;
constexpr float kCoinSheetY = 18.0F;  // rows of sprites.png: coins here
constexpr float kCoinSize = 8.0F;
constexpr float kHeartSheetY = 26.0F;  // heart icons (full, then empty) sit below the coins
constexpr float kHeartSize = 8.0F;
constexpr float kFruitSheetY = 34.0F;   // the glowing berry frames sit below the hearts
constexpr float kFruitFrameSize = 16.0F;  // the berry is 8x8, in the middle of its halo


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
      hero_(renderer, assets_dir + "sprites/hero.png"),
      decor_(renderer, assets_dir + "sprites/decor.png"),
      hazards_(renderer, assets_dir + "sprites/hazards.png"),
      enemies_(renderer, assets_dir + "sprites/enemies.png"),
      fireflies_(renderer, assets_dir + "sprites/fireflies.png"),
      checkpoints_(renderer, assets_dir + "sprites/checkpoints.png"),
      exit_(renderer, assets_dir + "sprites/exit.png"),
      orbs_(renderer, assets_dir + "sprites/orbs.png"),
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

    // A hit shakes the world (and the HUD), not the pause menu: shift the whole picture by
    // moving the viewport; the strip it uncovers is left black.
    const bool shaking = flow.screen() == application::Screen::Playing;
    const ShakeOffset shake = shaking ? screenShake(session->player().invulnerable_steps, domain::kInvulnerableSteps)
                                      : ShakeOffset{};
    if (shake.x != 0 || shake.y != 0) {
        SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
        SDL_RenderClear(renderer_);
        const SDL_Rect viewport{shake.x, shake.y, 320, 240};
        SDL_SetRenderViewport(renderer_, &viewport);
    }
    drawWorld(*session, time_ms);
    SDL_SetRenderViewport(renderer_, nullptr);

    if (flow.screen() == application::Screen::Won) {
        // The ending: the cave, the result and a menu.
        drawCave();
        drawText("YOU ESCAPED", 160.0F + 2.0F, 52.0F + 2.0F, 3.0F, SDL_Color{20, 16, 40, 255});
        drawText("YOU ESCAPED", 160.0F, 52.0F, 3.0F, SDL_Color{255, 240, 150, 255});
        drawText("COINS " + std::to_string(session->collectedCoins().count()) + "/" +
                     std::to_string(session->totalCoins()),
                 160.0F, 98.0F, 1.0F, SDL_Color{255, 232, 130, 255});
        drawMenu(flow, "", 1.0F, 118.0F - 8.0F - 24.0F);
        SDL_RenderPresent(renderer_);
        return;
    }
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
    drawScenery(session.currentRoom(), time_ms);
    drawFireflies(session, time_ms);
    drawHazards(session);
    drawExits(session, time_ms);
    drawOrbs(session, time_ms);
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

// The scenery of the room (see decoration.h): it sticks out of the rock, one pixel into it so
// that its outline merges with the rock's. The clusters of stalactites drip.
void GameRenderer::drawScenery(const domain::Room& room, std::uint64_t time_ms) {
    if (scenery_room_ != room.id()) {
        scenery_room_ = room.id();
        scenery_ = sceneryFor(room);
    }
    const float size = static_cast<float>(room.map().tileSize());
    for (const Prop& prop : scenery_) {
        const PropShape shape = propShape(prop.type);
        const float width = static_cast<float>(shape.width) * size;
        const float height = static_cast<float>(shape.height) * size;
        SDL_FRect destination{static_cast<float>(prop.column) * size, static_cast<float>(prop.row) * size,
                              width, height};
        if (shape.attach == Attach::Floor) {
            destination.y += 1.0F;
        } else if (shape.attach == Attach::Ceiling) {
            destination.y -= 1.0F;
        } else {
            destination.x += prop.mirrored ? 1.0F : -1.0F;
        }
        const SDL_FRect source{static_cast<float>(static_cast<int>(prop.type) * kPropCellSize), 0.0F, width, height};
        decor_.draw(renderer_, source, destination, prop.mirrored);
        if (prop.type == PropType::StalactiteCluster) {
            drawDrip(room.map(), prop, destination.y, time_ms);
        }
    }
}

// The water drop of a stalactite cluster whose top is at `top`: it forms at the longest tip, falls
// until it meets the first solid tile below (or the bottom of the screen) and splashes there. Only
// decoration: nothing in the game reacts to it.
void GameRenderer::drawDrip(const domain::TileMap& map, const Prop& prop, float top,
                            std::uint64_t time_ms) const {
    constexpr float kDrop = 8.0F;             // the drop frames are 8x8
    constexpr float kDropTopInFrame = 5.0F;   // where the drop starts inside its frame
    constexpr float kDropCentreInFrame = 4.0F;
    constexpr float kDropHeight = 2.0F;

    const float size = static_cast<float>(map.tileSize());
    const float x = static_cast<float>(prop.column) * size + static_cast<float>(kDripTipX) - kDropCentreInFrame;
    const float forming_y = top + static_cast<float>(kDripTipY) - kDropTopInFrame;
    const int tip_column = prop.column + kDripTipX / map.tileSize();
    int ground_row = prop.row + propShape(prop.type).height;
    while (ground_row < map.height() && !map.isGround(tip_column, ground_row)) {
        ++ground_row;
    }
    const float ground_y = static_cast<float>(ground_row) * size;
    const float fall_distance = ground_y - (forming_y + kDropTopInFrame + kDropHeight);

    const Drip drip = dripAt(prop.column, prop.row, fall_distance, time_ms);
    if (drip.phase == DripPhase::None) {
        return;
    }
    float y = forming_y;
    if (drip.phase == DripPhase::Falling) {
        y += drip.fall_offset;
    } else if (drip.phase == DripPhase::Splash) {
        y = ground_y - kDrop;  // the splash frames end in their bottom row
    }
    const SDL_FRect source{static_cast<float>(drip.frame) * kDrop, static_cast<float>(kPropCellSize), kDrop, kDrop};
    decor_.draw(renderer_, source, SDL_FRect{x, y, kDrop, kDrop});
}

// The orbs of the room, floating and beating; where one has been taken, its dashed silhouette.
void GameRenderer::drawOrbs(const application::GameSession& session, std::uint64_t time_ms) const {
    const domain::Room& room = session.currentRoom();
    for (std::size_t index = 0; index < room.orbs().size(); ++index) {
        const domain::Orb& orb = room.orbs()[index];
        const bool taken = session.collectedOrbs().isCollected(room.id(), index);
        const int column = taken ? kOrbFrames : orbFrame(orb.colour, time_ms);
        const float bob = taken ? 0.0F : static_cast<float>(orbBob(orb.colour, time_ms));
        const SDL_FRect source{static_cast<float>(column) * kOrbCell,
                               static_cast<float>(static_cast<int>(orb.colour)) * kOrbCell, kOrbCell, kOrbCell};
        // The cell is bigger than the hitbox (the ball swells and glows): centred on it.
        const float margin = (kOrbCell - orb.size) * 0.5F;
        orbs_.draw(renderer_, source, SDL_FRect{orb.x - margin, orb.y - margin + bob, kOrbCell, kOrbCell});
    }
}

// The cave exit: an archway standing on the floor, centred on its hitbox. A portcullis shuts it
// until every orb is taken; walking into its room with all of them, it rises.
void GameRenderer::drawExits(const application::GameSession& session, std::uint64_t time_ms) {
    constexpr float kWidth = 22.0F;
    constexpr float kHeight = 32.0F;
    const domain::Room& room = session.currentRoom();
    const bool on_screen = session.exitOpen() && !room.exits().empty();
    if (!on_screen) {
        gate_room_.clear();  // leaving the room: the next visit raises it again
    } else if (gate_room_ != room.id()) {
        gate_room_ = room.id();
        gate_open_since_ms_ = time_ms;
    }
    const std::uint64_t ms_open = on_screen ? time_ms - gate_open_since_ms_ : 0;
    for (const domain::Exit& exit : room.exits()) {
        const domain::AABB box = exit.bounds();
        const int frame = exitSpriteFrame(session.exitOpen(), ms_open, time_ms);
        exit_.draw(renderer_, SDL_FRect{static_cast<float>(frame) * kWidth, 0.0F, kWidth, kHeight},
                   SDL_FRect{box.x + (box.width - kWidth) * 0.5F, box.bottom() - kHeight, kWidth, kHeight});
    }
}

// Checkpoints: a dark crystal lantern until touched; the active one glows (two frames that pulse).
void GameRenderer::drawCheckpoints(const application::GameSession& session, std::uint64_t time_ms) const {
    constexpr float kWidth = 12.0F;  // taller than the hitbox: it stands on the floor, centred
    constexpr float kHeight = 20.0F;
    const domain::Room& room = session.currentRoom();
    for (std::size_t index = 0; index < room.checkpoints().size(); ++index) {
        const domain::AABB box = room.checkpoints()[index].bounds();
        const bool active = session.lastCheckpoint() &&
                            *session.lastCheckpoint() == std::make_pair(room.id(), index);
        const int frame = active ? checkpointFrame(time_ms) : 0;
        checkpoints_.draw(renderer_, SDL_FRect{static_cast<float>(frame) * kWidth, 0.0F, kWidth, kHeight},
                          SDL_FRect{box.x + (box.width - kWidth) * 0.5F, box.bottom() - kHeight, kWidth, kHeight});
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

// Spikes: the hazard's rectangle is the hitbox, and the 8x16 spike sprite is
// repeated along it. The sprite is drawn one pixel lower than the rectangle's
// bottom edge: its last row is a dark line that merges with the outline of the
// rock below, so the points look stuck in the ground.
void GameRenderer::drawHazards(const application::GameSession& session) const {
    constexpr float kSprite = 8.0F;
    constexpr float kSpikeHeight = 16.0F;
    constexpr float kEmbed = 1.0F;  // how many pixels the sprite sinks into the ground
    const domain::Room& room = session.currentRoom();
    for (const domain::Hazard& hazard : room.hazards()) {
        const domain::AABB& box = hazard.bounds;
        for (float x = box.x; x < box.right(); x += kSprite) {
            const float width = std::min(kSprite, box.right() - x);
            hazards_.draw(renderer_, SDL_FRect{0.0F, 0.0F, width, kSpikeHeight},
                          SDL_FRect{x, box.bottom() - kSpikeHeight + kEmbed, width, kSpikeHeight});
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
    constexpr float kSprite = 16.0F;
    const domain::Room& room = session.currentRoom();
    for (std::size_t index = 0; index < room.enemies().size(); ++index) {
        const domain::EnemyState& state = session.enemyPatrols().state(room.id(), index);
        const domain::AABB box = domain::currentBounds(room.enemies()[index], state);
        // The sprite is 16x16: sit it on the bottom of the hitbox.
        const SDL_FRect destination{box.x, box.bottom() - kSprite, kSprite, kSprite};
        if (!state.alive) {
            // The last frame: squashed flat. It blinks for its last third of a second, then it is gone.
            if (domain::isSquashedBody(state) && isVisibleWhileFading(domain::kSquashSteps - state.dead_steps)) {
                const SDL_FRect squashed{static_cast<float>(kEnemyWalkFrames) * kSprite, 0.0F, kSprite, kSprite};
                enemies_.draw(renderer_, squashed, destination, state.direction < 0);
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

    // One column per pose, one row per moment of the hood's wave.
    const SDL_FRect source{static_cast<float>(static_cast<int>(pose)) * kPlayerSpriteWidth,
                           static_cast<float>(hoodFlutter(pose, time_ms)) * kPlayerSpriteHeight,
                           kPlayerSpriteWidth, kPlayerSpriteHeight};
    const domain::AABB box = player.bounds();
    const SDL_FRect destination{box.x + (box.width - kPlayerSpriteWidth) * 0.5F,
                                box.bottom() - kPlayerSpriteHeight + kPlayerSoleOverhang,
                                kPlayerSpriteWidth, kPlayerSpriteHeight};
    if (!isVisibleWhileInvulnerable(player.invulnerable_steps)) {
        return;  // blinking after a hit
    }
    hero_.draw(renderer_, source, destination, facing_left_);
}

void GameRenderer::drawHud(const application::GameSession& session, std::uint64_t time_ms) const {
    const SDL_FRect icon_source{static_cast<float>(coinFrame(time_ms)) * kCoinSize, kCoinSheetY,
                                kCoinSize, kCoinSize};
    sprites_.draw(renderer_, icon_source, SDL_FRect{6.0F, 5.0F, kCoinSize, kCoinSize});

    const std::string text = std::to_string(session.collectedCoins().count()) + "/" +
                             std::to_string(session.totalCoins());
    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
    SDL_RenderDebugText(renderer_, 18.0F, 5.0F, text.c_str());

    // The orbs, top centre: a small ball for each one taken, its dashed silhouette until then.
    if (session.totalOrbs() > 0) {
        constexpr float kIcon = 11.0F;
        constexpr float kIconRow = 3.0F * kOrbCell;  // the icons are below the three rows of cells
        constexpr float kGap = 3.0F;
        constexpr int kColours = 3;
        const float first = (320.0F - kColours * kIcon - (kColours - 1) * kGap) * 0.5F;
        for (int colour = 0; colour < kColours; ++colour) {
            const bool taken = session.hasTakenOrb(static_cast<domain::OrbColour>(colour));
            const int column = taken ? colour : kColours + colour;
            orbs_.draw(renderer_, SDL_FRect{static_cast<float>(column) * kIcon, kIconRow, kIcon, kIcon},
                       SDL_FRect{first + static_cast<float>(colour) * (kIcon + kGap), 3.0F, kIcon, kIcon});
        }
    }

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
