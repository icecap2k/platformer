#pragma once

#include <SDL3/SDL.h>

#include "application/game_session.h"

namespace presentation {

// Draws the current state of a session. Read-only: it never changes the game.
void drawGame(SDL_Renderer* renderer, const application::GameSession& session);

}  // namespace presentation
