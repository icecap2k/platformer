#pragma once

namespace domain {

// What the player asked of the menus, one press at a time (not held keys, unlike
// InputState). The keyboard maps its keys to these, a future gamepad would map its
// buttons; the game flow (application/game_flow.h) decides what each one means on
// the current screen.
enum class MenuCommand {
    Up,
    Down,
    Confirm,  // pick the selected item
    Back,     // Esc: leave the pause menu, or quit from the title
    Pause,    // P: pause or resume
};

}  // namespace domain
