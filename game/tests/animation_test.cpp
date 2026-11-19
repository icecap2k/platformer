#include <doctest/doctest.h>

#include "presentation/animation.h"

using presentation::PlayerPose;

namespace {

domain::Player playerWith(float velocity_x, bool on_ground, float velocity_y = 0.0F) {
    domain::Player player;
    player.velocity_x = velocity_x;
    player.velocity_y = velocity_y;
    player.on_ground = on_ground;
    return player;
}

int column(PlayerPose pose) { return static_cast<int>(pose); }

}  // namespace

TEST_CASE("playerPose") {
    SUBCASE("every pose is a distinct column inside the sprite sheet") {
        const PlayerPose all[] = {PlayerPose::Idle0, PlayerPose::Idle1, PlayerPose::Idle2,
                                  PlayerPose::Idle3, PlayerPose::Run0,  PlayerPose::Run1,
                                  PlayerPose::Run2,  PlayerPose::Run3,  PlayerPose::Run4,
                                  PlayerPose::Run5,  PlayerPose::Rise,  PlayerPose::Apex,
                                  PlayerPose::Fall,  PlayerPose::Land, PlayerPose::Hurt};
        int seen = 0;
        for (const PlayerPose pose : all) {
            CHECK(column(pose) == seen);  // contiguous, in order
            ++seen;
        }
        CHECK(seen == presentation::kPlayerFrameCount);
    }

    SUBCASE("standing still cycles through the four idle frames") {
        const domain::Player still = playerWith(0.0F, true);

        CHECK(presentation::playerPose(still, 0) == PlayerPose::Idle0);
        CHECK(presentation::playerPose(still, 219) == PlayerPose::Idle0);
        CHECK(presentation::playerPose(still, 220) == PlayerPose::Idle1);
        CHECK(presentation::playerPose(still, 440) == PlayerPose::Idle2);
        CHECK(presentation::playerPose(still, 660) == PlayerPose::Idle3);
        CHECK(presentation::playerPose(still, 880) == PlayerPose::Idle0);
    }

    SUBCASE("running cycles through the six run frames") {
        const domain::Player running = playerWith(1.5F, true);

        CHECK(presentation::playerPose(running, 0) == PlayerPose::Run0);
        CHECK(presentation::playerPose(running, 79) == PlayerPose::Run0);
        CHECK(presentation::playerPose(running, 80) == PlayerPose::Run1);
        CHECK(presentation::playerPose(running, 400) == PlayerPose::Run5);
        CHECK(presentation::playerPose(running, 480) == PlayerPose::Run0);
    }

    SUBCASE("running left animates the same way") {
        CHECK(presentation::playerPose(playerWith(-1.5F, true), 80) == PlayerPose::Run1);
    }

    SUBCASE("in the air the pose follows the vertical speed") {
        CHECK(presentation::playerPose(playerWith(0.0F, false, -5.0F), 0) == PlayerPose::Rise);
        CHECK(presentation::playerPose(playerWith(0.0F, false, 0.0F), 0) == PlayerPose::Apex);
        CHECK(presentation::playerPose(playerWith(1.5F, false, 0.6F), 0) == PlayerPose::Apex);
        CHECK(presentation::playerPose(playerWith(0.0F, false, 4.0F), 0) == PlayerPose::Fall);
    }

    SUBCASE("just after landing, standing still, the squashed pose is shown briefly") {
        const domain::Player still = playerWith(0.0F, true);

        CHECK(presentation::playerPose(still, 1000, 0) == PlayerPose::Land);
        CHECK(presentation::playerPose(still, 1000, 99) == PlayerPose::Land);
        CHECK(presentation::playerPose(still, 1000, 100) != PlayerPose::Land);
        CHECK(presentation::playerPose(still, 1000) != PlayerPose::Land);
    }

    SUBCASE("being thrown back shows the hurt pose, whatever else is going on") {
        domain::Player hurt = playerWith(-1.2F, false, -2.0F);
        hurt.hurt_steps = 5;
        CHECK(presentation::playerPose(hurt, 0) == PlayerPose::Hurt);

        hurt.on_ground = true;
        hurt.velocity_x = 0.0F;
        CHECK(presentation::playerPose(hurt, 0, 0) == PlayerPose::Hurt);  // even right after landing

        hurt.hurt_steps = 0;  // recovered
        CHECK(presentation::playerPose(hurt, 0) != PlayerPose::Hurt);
    }

    SUBCASE("landing while running does not squash: the run continues") {
        const PlayerPose pose = presentation::playerPose(playerWith(1.5F, true), 0, 0);
        CHECK(pose == PlayerPose::Run0);
    }
}

TEST_CASE("isVisibleWhileInvulnerable") {
    SUBCASE("always visible when not invulnerable") {
        CHECK(presentation::isVisibleWhileInvulnerable(0));
        CHECK(presentation::isVisibleWhileInvulnerable(-1));
    }

    SUBCASE("blinks while invulnerable, hidden half of the time") {
        int hidden = 0;
        for (int steps = 1; steps <= 90; ++steps) {
            hidden += presentation::isVisibleWhileInvulnerable(steps) ? 0 : 1;
        }
        CHECK(hidden >= 40);
        CHECK(hidden <= 50);
        // and it really alternates in short bursts
        CHECK(presentation::isVisibleWhileInvulnerable(6) != presentation::isVisibleWhileInvulnerable(3));
    }
}

TEST_CASE("a player thrown back keeps facing what hit it") {
    domain::Player hurt = playerWith(1.2F, false);  // thrown to the right...
    hurt.hurt_steps = 5;
    CHECK(presentation::facesLeft(hurt, true));    // ...but still looking left
    CHECK_FALSE(presentation::facesLeft(hurt, false));
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
