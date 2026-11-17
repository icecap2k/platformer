#include <doctest/doctest.h>

#include "presentation/animation.h"

using presentation::PlayerPose;

namespace {

domain::Player playerWith(float velocity_x, bool on_ground) {
    domain::Player player;
    player.velocity_x = velocity_x;
    player.on_ground = on_ground;
    return player;
}

}  // namespace

TEST_CASE("playerPose") {
    SUBCASE("standing still on the ground is idle") {
        CHECK(presentation::playerPose(playerWith(0.0F, true), 0) == PlayerPose::Idle);
        CHECK(presentation::playerPose(playerWith(0.0F, true), 5000) == PlayerPose::Idle);
    }

    SUBCASE("being in the air always shows the jump pose") {
        CHECK(presentation::playerPose(playerWith(0.0F, false), 0) == PlayerPose::Jump);
        CHECK(presentation::playerPose(playerWith(1.5F, false), 0) == PlayerPose::Jump);
    }

    SUBCASE("walking alternates between the two walk frames over time") {
        const domain::Player walking = playerWith(1.5F, true);

        CHECK(presentation::playerPose(walking, 0) == PlayerPose::Walk1);
        CHECK(presentation::playerPose(walking, 119) == PlayerPose::Walk1);
        CHECK(presentation::playerPose(walking, 120) == PlayerPose::Walk2);
        CHECK(presentation::playerPose(walking, 240) == PlayerPose::Walk1);
    }

    SUBCASE("walking left animates the same way") {
        CHECK(presentation::playerPose(playerWith(-1.5F, true), 120) == PlayerPose::Walk2);
    }
}

TEST_CASE("facesLeft") {
    CHECK(presentation::facesLeft(playerWith(-1.5F, true), false));
    CHECK_FALSE(presentation::facesLeft(playerWith(1.5F, true), true));

    SUBCASE("keeps the last direction while standing still") {
        CHECK(presentation::facesLeft(playerWith(0.0F, true), true));
        CHECK_FALSE(presentation::facesLeft(playerWith(0.0F, true), false));
    }
}

TEST_CASE("coinFrame cycles through four frames") {
    CHECK(presentation::coinFrame(0) == 0);
    CHECK(presentation::coinFrame(140) == 1);
    CHECK(presentation::coinFrame(280) == 2);
    CHECK(presentation::coinFrame(420) == 3);
    CHECK(presentation::coinFrame(560) == 0);
}
