#pragma once

namespace domain {

// Axis-aligned bounding box: a rectangle that never rotates.
// (x, y) is the top-left corner; y grows downwards, like on screen.
struct AABB {
    float x = 0.0F;
    float y = 0.0F;
    float width = 0.0F;
    float height = 0.0F;

    float left() const { return x; }
    float right() const { return x + width; }
    float top() const { return y; }
    float bottom() const { return y + height; }

    // Boxes that only touch along an edge do NOT overlap.
    bool overlaps(const AABB& other) const {
        return left() < other.right() && right() > other.left() && top() < other.bottom() &&
               bottom() > other.top();
    }
};

}  // namespace domain
