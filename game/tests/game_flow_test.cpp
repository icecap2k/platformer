#include <doctest/doctest.h>

#include "application/game_flow.h"

using application::GameFlow;
using application::MenuItem;
using application::Screen;
using domain::MenuCommand;

namespace {
GameFlow playing() {
    GameFlow flow;
    REQUIRE(flow.handle(MenuCommand::Confirm));  // Start
    return flow;
}
}  // namespace

TEST_CASE("the game opens on the title screen with Start selected") {
    const GameFlow flow;
    CHECK(flow.screen() == Screen::Title);
    CHECK(flow.items() == std::vector<MenuItem>{MenuItem::Start, MenuItem::Quit});
    CHECK(flow.selected() == 0);
    CHECK_FALSE(flow.quitRequested());
}

TEST_CASE("Start begins a new game, and only Start asks for one") {
    GameFlow flow;
    CHECK(flow.handle(MenuCommand::Confirm));
    CHECK(flow.screen() == Screen::Playing);
    CHECK(flow.items().empty());

    GameFlow other;
    CHECK_FALSE(other.handle(MenuCommand::Down));
    CHECK_FALSE(other.handle(MenuCommand::Up));
    CHECK_FALSE(other.handle(MenuCommand::Pause));
}

TEST_CASE("the selection moves and wraps around") {
    GameFlow flow;
    flow.handle(MenuCommand::Down);
    CHECK(flow.selected() == 1);
    flow.handle(MenuCommand::Down);
    CHECK(flow.selected() == 0);  // wraps past Quit back to Start
    flow.handle(MenuCommand::Up);
    CHECK(flow.selected() == 1);  // and up from Start goes to Quit
}

TEST_CASE("Quit on the title, or Esc there, asks to leave") {
    GameFlow by_item;
    by_item.handle(MenuCommand::Down);
    CHECK_FALSE(by_item.handle(MenuCommand::Confirm));
    CHECK(by_item.quitRequested());

    GameFlow by_escape;
    by_escape.handle(MenuCommand::Back);
    CHECK(by_escape.quitRequested());
}

TEST_CASE("pausing and resuming") {
    SUBCASE("P and Esc pause the game") {
        GameFlow a = playing();
        a.handle(MenuCommand::Pause);
        CHECK(a.screen() == Screen::Paused);
        GameFlow b = playing();
        b.handle(MenuCommand::Back);
        CHECK(b.screen() == Screen::Paused);
    }
    SUBCASE("the pause menu offers continue, back to title and quit, with Continue selected") {
        GameFlow flow = playing();
        flow.handle(MenuCommand::Down);  // meaningless while playing: must not preselect anything
        flow.handle(MenuCommand::Pause);
        CHECK(flow.items() ==
              std::vector<MenuItem>{MenuItem::Continue, MenuItem::BackToTitle, MenuItem::Quit});
        CHECK(flow.selected() == 0);
    }
    SUBCASE("P, Esc and Continue all resume, without starting a new game") {
        GameFlow flow = playing();
        flow.handle(MenuCommand::Pause);
        flow.handle(MenuCommand::Pause);
        CHECK(flow.screen() == Screen::Playing);

        flow.handle(MenuCommand::Back);
        flow.handle(MenuCommand::Back);
        CHECK(flow.screen() == Screen::Playing);

        flow.handle(MenuCommand::Pause);
        CHECK_FALSE(flow.handle(MenuCommand::Confirm));  // Continue
        CHECK(flow.screen() == Screen::Playing);
        CHECK_FALSE(flow.quitRequested());
    }
    SUBCASE("Back to title leaves the game and a later Start asks for a fresh one") {
        GameFlow flow = playing();
        flow.handle(MenuCommand::Pause);
        flow.handle(MenuCommand::Down);
        CHECK_FALSE(flow.handle(MenuCommand::Confirm));  // Back to title
        CHECK(flow.screen() == Screen::Title);
        CHECK(flow.selected() == 0);
        CHECK(flow.handle(MenuCommand::Confirm));        // Start: a new game
    }
    SUBCASE("Quit from the pause menu") {
        GameFlow flow = playing();
        flow.handle(MenuCommand::Pause);
        flow.handle(MenuCommand::Up);  // wraps to Quit
        flow.handle(MenuCommand::Confirm);
        CHECK(flow.quitRequested());
    }
}

TEST_CASE("winning shows the ending, with back to title and quit") {
    GameFlow flow = playing();
    flow.win();
    CHECK(flow.screen() == Screen::Won);
    CHECK(flow.items() == std::vector<MenuItem>{MenuItem::BackToTitle, MenuItem::Quit});
    CHECK(flow.selected() == 0);

    SUBCASE("P does nothing there") {
        flow.handle(MenuCommand::Pause);
        CHECK(flow.screen() == Screen::Won);
    }
    SUBCASE("Back to title, then Start asks for a fresh game") {
        CHECK_FALSE(flow.handle(MenuCommand::Confirm));
        CHECK(flow.screen() == Screen::Title);
        CHECK(flow.handle(MenuCommand::Confirm));
    }
    SUBCASE("Esc also goes back to the title") {
        flow.handle(MenuCommand::Back);
        CHECK(flow.screen() == Screen::Title);
        CHECK_FALSE(flow.quitRequested());
    }
    SUBCASE("Quit") {
        flow.handle(MenuCommand::Down);
        flow.handle(MenuCommand::Confirm);
        CHECK(flow.quitRequested());
    }
}

TEST_CASE("every menu item has a label") {
    for (const MenuItem item : {MenuItem::Start, MenuItem::Continue, MenuItem::BackToTitle, MenuItem::Quit}) {
        CHECK_FALSE(application::label(item).empty());
    }
}
