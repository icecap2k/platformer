#pragma once

#include <string>
#include <vector>

#include "domain/input/menu_command.h"

namespace application {

// Which screen the player is looking at. GameFlow sits above GameSession: the session is
// one play-through and knows nothing about menus; the flow decides when there is one,
// whether it advances (only while Playing) and when it is thrown away and started again.
enum class Screen {
    Title,    // the first screen: start or quit
    Playing,  // the game runs
    Paused,   // the game is frozen behind a menu
};

enum class MenuItem {
    Start,        // title: a new game
    Continue,     // pause: back to the game
    BackToTitle,  // pause: leave this game
    Quit,
};

// What the item says on screen.
std::string label(MenuItem item);

// The state of the menus. No SDL, so every screen change is unit tested.
class GameFlow {
public:
    Screen screen() const { return screen_; }

    // The items of the current screen's menu (empty while Playing).
    std::vector<MenuItem> items() const;
    int selected() const { return selected_; }
    bool quitRequested() const { return quit_requested_; }

    // Reacts to one command. Returns true when a NEW game has to be started (the player
    // picked Start): the caller creates a fresh GameSession.
    bool handle(domain::MenuCommand command);

private:
    void openMenu(Screen screen);
    bool choose(MenuItem item);

    Screen screen_ = Screen::Title;
    int selected_ = 0;
    bool quit_requested_ = false;
};

}  // namespace application
