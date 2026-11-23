#include "application/game_flow.h"

namespace application {

std::string label(MenuItem item) {
    switch (item) {
        case MenuItem::Start:
            return "START";
        case MenuItem::Continue:
            return "CONTINUE";
        case MenuItem::BackToTitle:
            return "BACK TO TITLE";
        case MenuItem::Quit:
            return "QUIT";
    }
    return "";
}

std::vector<MenuItem> GameFlow::items() const {
    switch (screen_) {
        case Screen::Title:
            return {MenuItem::Start, MenuItem::Quit};
        case Screen::Paused:
            return {MenuItem::Continue, MenuItem::BackToTitle, MenuItem::Quit};
        case Screen::Won:
            return {MenuItem::BackToTitle, MenuItem::Quit};
        case Screen::Playing:
            return {};
    }
    return {};
}

void GameFlow::openMenu(Screen screen) {
    screen_ = screen;
    selected_ = 0;
}

bool GameFlow::choose(MenuItem item) {
    switch (item) {
        case MenuItem::Start:
            screen_ = Screen::Playing;
            return true;
        case MenuItem::Continue:
            screen_ = Screen::Playing;
            return false;
        case MenuItem::BackToTitle:
            openMenu(Screen::Title);
            return false;
        case MenuItem::Quit:
            quit_requested_ = true;
            return false;
    }
    return false;
}

void GameFlow::win() {
    openMenu(Screen::Won);
}

bool GameFlow::handle(domain::MenuCommand command) {
    using domain::MenuCommand;
    if (screen_ == Screen::Playing) {
        if (command == MenuCommand::Pause || command == MenuCommand::Back) {
            openMenu(Screen::Paused);
        }
        return false;  // the other commands mean nothing while playing
    }

    const std::vector<MenuItem> menu = items();
    const int count = static_cast<int>(menu.size());
    switch (command) {
        case MenuCommand::Up:
            selected_ = (selected_ + count - 1) % count;
            return false;
        case MenuCommand::Down:
            selected_ = (selected_ + 1) % count;
            return false;
        case MenuCommand::Confirm:
            return choose(menu[static_cast<std::size_t>(selected_)]);
        case MenuCommand::Pause:
            if (screen_ == Screen::Paused) {
                screen_ = Screen::Playing;  // P again resumes
            }
            return false;
        case MenuCommand::Back:
            if (screen_ == Screen::Paused) {
                screen_ = Screen::Playing;  // Esc steps out of the pause menu...
            } else if (screen_ == Screen::Won) {
                openMenu(Screen::Title);    // ...and out of the ending to the title
            } else {
                quit_requested_ = true;     // ...and out of the game from the title
            }
            return false;
    }
    return false;
}

}  // namespace application
