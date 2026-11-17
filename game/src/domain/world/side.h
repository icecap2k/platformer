#pragma once

namespace domain {

// One of the four edges of a room.
enum class Side { North, South, East, West };

inline Side opposite(Side side) {
    switch (side) {
        case Side::North:
            return Side::South;
        case Side::South:
            return Side::North;
        case Side::East:
            return Side::West;
        case Side::West:
            return Side::East;
    }
    return side;  // unreachable; keeps compilers happy
}

}  // namespace domain
