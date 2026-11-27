#pragma once

#include <SDL3/SDL.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "application/game_flow.h"
#include "application/game_session.h"
#include "infrastructure/sdl/render_target.h"
#include "infrastructure/sdl/texture.h"
#include "presentation/backdrop.h"
#include "presentation/decoration.h"
#include "presentation/firefly.h"
#include "presentation/transition.h"

namespace presentation {

// Draws the current state of a session. It only reads the game, never changes
// it. The only state it owns is purely visual (loaded images and which way the
// player is facing).
class GameRenderer {
public:
    // `assets_dir` must end with a path separator and contain sprites/rocks.png,
    // sprites/sprites.png, sprites/hero.png, sprites/decor.png, sprites/hazards.png, sprites/enemies.png, sprites/bats.png, sprites/fireflies.png, sprites/checkpoints.png, sprites/exit.png, sprites/orbs.png, sprites/glow_mushrooms.png, background/cave.png and background/title.png. Throws std::runtime_error if they cannot be loaded.
    GameRenderer(SDL_Renderer* renderer, const std::string& assets_dir);

    // Draws the current screen: the title menu, or the game (frozen behind the pause menu
    // while paused). `session` may be null only while the flow is on the title screen.
    // Changing room or screen is drawn as an eased transition from the previous picture.
    void draw(const application::GameSession* session, const application::GameFlow& flow);

    // Tell it what happens in the game, so it knows how to go from one room to the next.
    void onEvent(const application::GameEvent& event);

    // Shows a short message at the bottom of the screen for a moment.
    void showNotice(const std::string& text);

    // Whether a gamepad is connected: the on-screen hints name its buttons then.
    void setGamepad(bool connected) { gamepad_ = connected; }

    // True if a drop of water splashed in the room since the last call (so it can be heard).
    bool takeDripSplash();

    // True while a transition is on its way: the game should not advance meanwhile.
    bool inTransition() const;

private:
    // The change asked for and not started yet (it starts with the next picture drawn).
    struct Change {
        TransitionKind kind = TransitionKind::None;
        domain::RoomId left_room;  // for a slide: the room the player came from
    };

    void startPendingTransition(const application::GameSession* session, std::uint64_t time_ms);
    // The picture behind the HUD: the title, the ending or the room.
    void drawScene(const application::GameSession* session, const application::GameFlow& flow, std::uint64_t time_ms);
    void composeScene(std::uint64_t time_ms) const;
    void drawWorld(const application::GameSession& session, std::uint64_t time_ms);
    void drawMenu(const application::GameFlow& flow, const std::string& heading, float heading_scale,
                  float top, float centre_x = 160.0F) const;
    void drawText(const std::string& text, float centre_x, float y, float scale, SDL_Color colour) const;
    void drawCave() const;
    void drawTitle(const application::GameFlow& flow) const;
    void drawBackdrop(const domain::Room& room, std::uint64_t time_ms);
    void drawTiles(const domain::TileMap& map) const;
    void drawScenery(const domain::Room& room, std::uint64_t time_ms);
    void drawDrip(const domain::TileMap& map, const Prop& prop, float top, std::uint64_t time_ms);
    void drawOrbs(const application::GameSession& session, std::uint64_t time_ms) const;
    void drawExits(const application::GameSession& session, std::uint64_t time_ms);
    void drawCheckpoints(const application::GameSession& session, std::uint64_t time_ms) const;
    void drawFireflies(const application::GameSession& session, std::uint64_t time_ms);
    void drawHazards(const application::GameSession& session) const;
    void drawEnemies(const application::GameSession& session, std::uint64_t time_ms) const;
    void drawFruits(const application::GameSession& session, std::uint64_t time_ms) const;
    void drawCoins(const application::GameSession& session, std::uint64_t time_ms) const;
    void drawPlayer(const domain::Player& player, std::uint64_t time_ms);
    void drawHud(const application::GameSession& session, std::uint64_t time_ms) const;

    SDL_Renderer* renderer_;
    infrastructure::Texture rocks_;
    infrastructure::Texture sprites_;
    infrastructure::Texture hero_;
    infrastructure::Texture decor_;
    infrastructure::Texture hazards_;
    infrastructure::Texture enemies_;
    infrastructure::Texture bats_;
    infrastructure::Texture fireflies_;
    infrastructure::Texture checkpoints_;
    infrastructure::Texture exit_;
    infrastructure::Texture orbs_;
    infrastructure::Texture mushrooms_;
    infrastructure::Texture cave_;
    infrastructure::Texture title_;
    // The scene is drawn off screen, and the one before the last change is kept to move away from.
    std::unique_ptr<infrastructure::RenderTarget> scene_;
    std::unique_ptr<infrastructure::RenderTarget> previous_scene_;
    Transition transition_;
    std::string notice_;
    std::uint64_t notice_until_ms_ = 0;
    // Listening for splashes: when drops were last looked at (0 = not listening).
    std::uint64_t drips_checked_ms_ = 0;
    bool drip_splashed_ = false;
    bool playing_ = false;
    bool gamepad_ = false;
    Change pending_;
    std::optional<application::Screen> last_screen_;
    // The fireflies' routes depend only on the room, so they are worked out once per room.
    std::string firefly_room_;
    std::vector<FireflyRoute> firefly_routes_;
    // When the open exit came on screen (its portcullis rises from then on); empty room = not on screen.
    std::string gate_room_;
    std::uint64_t gate_open_since_ms_ = 0;
    // Same for the mushrooms far behind.
    std::string backdrop_room_;
    std::vector<BackdropMushroom> backdrop_;
    // Same for the scenery.
    std::string scenery_room_;
    std::vector<Prop> scenery_;
    bool facing_left_ = false;
    bool was_on_ground_ = true;
    std::uint64_t landed_at_ms_ = 0;
    bool has_landed_ = false;
};

}  // namespace presentation
